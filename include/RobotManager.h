#pragma once

#include <mujoco/mujoco.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <memory>
#include <msgpack.hpp>
#include <msgpack/v3/object_fwd_decl.hpp>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Constants.h"
#include "MujocoContext.h"
#include "robots/BoosterK1.h"
#include "robots/BoosterT1.h"
#include "robots/Robot.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

struct Team;  // Forward declaration

class RobotManager {
   public:
    // Singleton class
    static RobotManager& instance() {
        static RobotManager mgr;
        return mgr;
    }

    void registerRobot(std::shared_ptr<Robot> robot) {
        std::lock_guard<std::mutex> lock(mutex_);

        robots_.push_back(std::move(robot));
    }

    std::vector<std::shared_ptr<Robot>> getRobots() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return robots_;
    }

    size_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return robots_.size();
    }

    void update() {
        std::lock_guard lock(mutex_);
        for (std::shared_ptr<Robot> r : robots_) {
            r->update();
        }
    }

    void clear() {
        std::lock_guard lock(mutex_);
        for (std::shared_ptr<Robot> r : robots_) {
            // Drop ownership first
            r->container.reset();
            r->team.reset();
        }
        robots_.clear();
    }

    void bindMujoco(MujocoContext* mujContext) {
        for (std::shared_ptr<Robot> r : robots_)
            r->bindMujoco(mujContext);
    }

    std::shared_ptr<Robot> create(const std::string& name, const std::string& type, uint8_t number,
                                  const Eigen::Vector3d& pos, const Eigen::Vector3d& ori,
                                  const std::shared_ptr<Team> team) {
        auto it = robotFactory.find(type);
        if (it != robotFactory.end())
            return it->second(name, type, number, pos, ori, team);
        return nullptr;
    }

    void startContainers() {
        startCommunicationServer(frameworkCommunicationPort);

        YAML::Node configRoot;
        try {
            configRoot = YAML::LoadFile(frameworkConfigPath);
        } catch (const YAML::BadFile& e) {
            throw std::runtime_error("Failed to open YAML file: " + std::string(frameworkConfigPath));
        } catch (const YAML::ParserException& e) {
            throw std::runtime_error("Failed to parse YAML file: " + std::string(e.what()));
        }

        if (!configRoot["image"])
            throw std::runtime_error("Missing 'image' key in YAML file");

        std::string image;
        try {
            image = configRoot["image"].as<std::string>();
        } catch (const YAML::Exception& e) {
            throw std::runtime_error("'image' must be a string: " + std::string(e.what()));
        }

        if (!configRoot["volumes"] || !configRoot["volumes"].IsSequence())
            throw std::runtime_error("'volumes' key missing or not a sequence");

        std::vector<std::string> binds;
        for (const auto& v : configRoot["volumes"]) {
            try {
                binds.push_back(v.as<std::string>());
            } catch (const YAML::Exception& e) {
                throw std::runtime_error("Volume entry must be a string: " + std::string(e.what()));
            }
        }

        for (std::shared_ptr<Robot> r : robots_) {
            r->container = std::make_unique<Container>(r->name + "_container");
            r->container->create(r->name, image, binds);
            r->container->start();
        }
    }

    void startCommunicationServer(int port) {
        if (serverRunning_)
            throw std::runtime_error("Server already running");
        serverRunning_ = true;
        serverThread_ = std::thread(&RobotManager::_serverInternal, this, port);
    }

    void stopCommunicationServer() {
        if (!serverRunning_)
            return;

        serverRunning_ = false;

        if (serverThread_.joinable())
            serverThread_.join();
    }

   private:
    RobotManager() = default;
    ~RobotManager() = default;

    RobotManager(const RobotManager&) = delete;
    RobotManager& operator=(const RobotManager&) = delete;

    void _serverInternal(int port) {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
            throw std::runtime_error("Failed to create socket");

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
            throw std::runtime_error("Socket bind failed");
        if (listen(server_fd, robots_.size()) < 0)
            throw std::runtime_error("Listen failed");

        std::vector<pollfd> fds;
        fds.push_back({server_fd, POLLIN, 0});

        // Using a polling server. It isn't immediately intuitive, but it is efficient for this use case.
        while (serverRunning_) {
            // the poll blocks until a new connection arrives on server_fd or data arrives in one of the
            // monitored fd or a socket closes or the timeout expires.
            int ret = poll(fds.data(), fds.size(), 100);
            if (ret <= 0)
                continue;  // Timeout, skip iteration (timeout necessary to check whether serverRunning_ is
                           // still true)

            for (size_t i = 0; i < fds.size(); ++i) {
                // An event occured for the i-th fd
                if (fds[i].revents & POLLIN) {
                    if (fds[i].fd == server_fd) {
                        // The only event for the server is someone knocking
                        int client_fd = accept(server_fd, nullptr, nullptr);
                        if (client_fd >= 0)
                            fds.push_back({client_fd, POLLIN, 0});
                    } else {
                        // The events for other fds indicate either an incoming message or a closed connection
                        // the read call disambiguates the two cases
                        char buffer[MAX_MSG_SIZE];
                        int n = read(fds[i].fd, buffer, sizeof(buffer) - 1);
                        if (n <= 0) {
                            close(fds[i].fd);
                            fds.erase(fds.begin() + i);
                            --i;
                            continue;
                        }

                        msgpack::object_handle oh = msgpack::unpack(buffer, n);
                        auto data_map = oh.get().as<std::map<std::string, msgpack::object>>();
                        auto it = data_map.find("robot_name");
                        if (it == data_map.end())
                            continue;

                        std::string messageRecipient = it->second.as<std::string>();

                        std::unique_lock lock(mutex_);
                        for (auto& r : robots_) {
                            if (r->name == messageRecipient) {
                                r->receiveMessage(data_map);
                                auto answ = r->sendMessage();
                                msgpack::sbuffer sbuf;
                                msgpack::pack(sbuf, answ);
                                send(fds[i].fd, sbuf.data(), sbuf.size(), 0);
                                break;
                            }
                        }
                    }
                }
            }
        }

        for (auto& fd : fds)
            close(fd.fd);
    }

    std::atomic<bool> serverRunning_ = false;
    std::thread serverThread_;

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Robot>> robots_;

    using RobotCreator = std::function<std::shared_ptr<Robot>(const std::string&, const std::string&, uint8_t,
                                                              const Eigen::Vector3d&, const Eigen::Vector3d&,
                                                              const std::shared_ptr<Team>&)>;

    std::unordered_map<std::string, RobotCreator> robotFactory
        = {{"Booster-K1",
            [](auto&& name, auto&& type, uint8_t number, auto&& pos, auto&& ori, auto&& team) {
                return std::make_shared<BoosterK1>(name, type, number, pos, ori, team);
            }},
           {"Booster-T1", [](auto&& name, auto&& type, uint8_t number, auto&& pos, auto&& ori, auto&& team) {
                return std::make_shared<BoosterT1>(name, type, number, pos, ori, team);
            }}};
};

}  // namespace spqr
