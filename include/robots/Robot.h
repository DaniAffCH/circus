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
#include <string>

#include "Container.h"
#include "MujocoContext.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

struct Team;  // Forward declaration

struct DebugMessage {
    std::string idLocal;
    std::string drawGeomType;

    std::array<double, 3> center
        = {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
           std::numeric_limits<double>::quiet_NaN()};
    std::array<double, 3> start
        = {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
           std::numeric_limits<double>::quiet_NaN()};
    std::array<double, 3> end
        = {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
           std::numeric_limits<double>::quiet_NaN()};

    double radius = -1;
    double thickness = -1;
    double length = -1;

    std::array<double, 4> color = {1.0, 1.0, 1.0, 1.0};

    bool remove = false;
};

class Robot {
   public:
    Robot(const std::string& name, const std::string& type, uint8_t number,
          const Eigen::Vector3d& initPosition, const Eigen::Vector3d& initOrientation,
          const std::shared_ptr<Team>& team)
        : name(name),
          type(type),
          number(number),
          initPosition(initPosition),
          initOrientation(initOrientation),
          team(team) {}
    virtual ~Robot() = default;
    virtual void bindMujoco(MujocoContext* mujContext) = 0;
    virtual void update() = 0;
    virtual void receiveMessage(const std::map<std::string, msgpack::object>& message) = 0;
    virtual std::map<std::string, msgpack::object> sendMessage() = 0;

    std::map<std::string, msgpack::object> sendDebugMessage(DebugMessage debugMessage) {
        buffer_zone_.clear();
        std::map<std::string, msgpack::object> msg;
        msg["robot_name"] = msgpack::object(name, buffer_zone_);
        msg["idLocal"] = msgpack::object(debugMessage.idLocal, buffer_zone_);
        msg["drawGeomType"] = msgpack::object(debugMessage.drawGeomType, buffer_zone_);
        msg["center"] = msgpack::object(debugMessage.center, buffer_zone_);
        msg["radius"] = msgpack::object(debugMessage.radius, buffer_zone_);
        msg["start"] = msgpack::object(debugMessage.start, buffer_zone_);
        msg["end"] = msgpack::object(debugMessage.end, buffer_zone_);
        msg["color"] = msgpack::object(debugMessage.color, buffer_zone_);

        return msg;
    }

    std::string name;
    std::string type;
    uint8_t number;
    Eigen::Vector3d initPosition;
    Eigen::Vector3d initOrientation;  // Euler angles
    std::unique_ptr<Container> container;
    std::shared_ptr<Team> team;

    msgpack::zone buffer_zone_;
};

}  // namespace spqr
