#pragma once

#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <memory>
#include <pugixml.hpp>
#include <string>
#include <unordered_set>

#include "Team.h"
#include "robots/Robot.h"

using namespace pugi;
using namespace std;
using namespace Eigen;

namespace spqr {

struct BallSpec {
    Vector3d position;
};

struct CellData {
    int row;
    int column;
    string stream;
};

struct GuiConfig {
    int rows = 1;
    int columns = 4;
    std::vector<CellData> cellData;
};

struct SceneSpec {
    std::string field;
    std::vector<std::shared_ptr<Team>> teams;
    GuiConfig guiConfig;
};

class SceneParser {
   public:
    SceneParser(const string& yamlPath);
    string buildMuJoCoXml();
    const SceneSpec& getSceneInfo() const {
        return scene;
    }

   private:
    void buildRobotCommon(const string& robotType, xml_node& mujoco);
    void buildRobotInstance(const shared_ptr<Robot>& robotSpec, xml_node& worldbody, xml_node& actuator,
                            xml_node& sensor);
    void prefixSubtree(xml_node& root, const std::string& robotName);

    unordered_set<string> robotTypes;
    YAML::Node sceneRoot;
    SceneSpec scene;
    BallSpec ballSpec;
};

}  // namespace spqr
