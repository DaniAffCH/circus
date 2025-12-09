#pragma once
#include <memory>
#include <string>
#include <vector>

#include "RobotManager.h"
#include "robots/Robot.h"

namespace spqr {

struct Team {
    std::string name;
    std::vector<std::shared_ptr<Robot>> robots;
};

class TeamManager {
   public:
    // Singleton class
    static TeamManager& instance() {
        static TeamManager mgr;
        return mgr;
    }

    void registerTeam(std::shared_ptr<Team> team) {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const std::shared_ptr<Robot>& robot : team->robots)
            RobotManager::instance().registerRobot(robot);

        teams_.push_back(std::move(team));
    }

    std::vector<std::shared_ptr<Team>> getTeams() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return teams_;
    }

    size_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return teams_.size();
    }

    void clear() {
        std::lock_guard lock(mutex_);
        RobotManager::instance().clear();

        for (const std::shared_ptr<Team>& team : teams_)
            team->robots.clear();

        teams_.clear();
    }

   private:
    TeamManager() = default;
    ~TeamManager() = default;

    TeamManager(const TeamManager&) = delete;
    TeamManager& operator=(const TeamManager&) = delete;

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Team>> teams_;
};

}  // namespace spqr
