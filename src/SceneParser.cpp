#include "SceneParser.h"

#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <stdexcept>

#include "RobotManager.h"
#include "Team.h"
#include "robots/Robot.h"

using namespace pugi;
using namespace std;
namespace spqr {

SceneParser::SceneParser(const string& yamlPath) {
    sceneRoot = YAML::LoadFile(yamlPath);

    if (!sceneRoot["field"])
        throw runtime_error("Scene missing 'field' entry.");
    scene.field = sceneRoot["field"].as<string>();

    if (sceneRoot["ball"] && sceneRoot["ball"]["position"]) {
        for (int i = 0; i < 3; ++i)
            ballSpec.position[i] = sceneRoot["ball"]["position"][i].as<double>();
    } else {
        ballSpec.position = Eigen::Vector3d(0.0, 0.0, 0.12);
    }

    // Load GUI configuration parameters
    if (sceneRoot["gui_config"]) {
        const YAML::Node& guiConfigNode = sceneRoot["gui_config"];
        if (!guiConfigNode.IsSequence()  || guiConfigNode.size() == 0) {
            throw runtime_error("The GUI configuration must be a sequence and contain at least one element.");
        }
        const YAML::Node& toolsPanelNode = guiConfigNode[0]["tools_panel"];
        if (toolsPanelNode && toolsPanelNode.IsSequence() && toolsPanelNode.size() >= 2) {
                scene.guiConfig.rows = toolsPanelNode[0].as<int>();
                scene.guiConfig.columns = toolsPanelNode[1].as<int>();
            }
        const YAML::Node& cellDataNode = guiConfigNode[1]["cell_data"];
        if (cellDataNode && cellDataNode.IsSequence()) {
            if (cellDataNode.size() > scene.guiConfig.rows * scene.guiConfig.columns) {
                throw runtime_error("The number of cell data entries must be less than or equal to the total number of GUI cells.");
            }
            for (const YAML::Node& cellNode : cellDataNode) {
                CellData cellData;
                if (cellNode["cell"] && cellNode["cell"].IsSequence() && cellNode["cell"].size() >= 2) {
                    cellData.row = cellNode["cell"][0].as<int>();
                    cellData.column = cellNode["cell"][1].as<int>();
                }
                if (cellNode["stream"])
                    cellData.stream = cellNode["stream"].as<string>();
                scene.guiConfig.cellData.push_back(cellData);
            }
        }
    }

    const YAML::Node& teamsNode = sceneRoot["teams"];
    if (!teamsNode || teamsNode.size() > 2) {
        throw runtime_error("Scene must contain one or two teams.");
    }

    for (const auto& team : teamsNode) {
        const string& teamName = team.first.as<string>();
        const YAML::Node& robotsNode = team.second;

        if (!robotsNode.IsSequence())
            throw runtime_error("Each team must be a sequence of robots.");

        shared_ptr<Team> teamSpec = std::make_shared<Team>();
        teamSpec->name = teamName;

        uint8_t typeIndex = 0;
        for (const YAML::Node& robotNode : robotsNode) {
            if (!robotNode["type"])
                throw runtime_error("Robot missing type field.");
            if (!robotNode["number"])
                throw runtime_error("Robot missing jersey number");

            string robotType = robotNode["type"].as<string>();  // complete name <Brand>-<Model>
            uint8_t robotNumber = robotNode["number"].as<uint8_t>();
            string robotName = robotNode["name"] ? robotNode["name"].as<string>() :
                                                   teamName + "_" + robotType + "_" + to_string(typeIndex++);
            Vector3d pos = Vector3d::Zero();
            Vector3d ori = Vector3d::Zero();

            if (robotNode["position"]) {
                for (int i = 0; i < 3; ++i)
                    pos[i] = robotNode["position"][i].as<double>();
            }

            if (robotNode["orientation"]) {
                for (int i = 0; i < 3; ++i)
                    ori[i] = robotNode["orientation"][i].as<double>();
            }

            shared_ptr<Robot> robot
                = RobotManager::instance().create(robotName, robotType, robotNumber, pos, ori, teamSpec);

            robotTypes.insert(robotType);
            teamSpec->robots.push_back(std::move(robot));
        }

        scene.teams.push_back(teamSpec);
        TeamManager::instance().registerTeam(teamSpec);
    }
}

string SceneParser::buildMuJoCoXml() {
    xml_document doc;

    xml_node mujoco = doc.append_child("mujoco");

    // TODO: The simulation options can be parametrized. I don't know if we may want to change the parameters.
    xml_node option = mujoco.append_child("option");
    option.append_attribute("timestep") = "0.01";
    option.append_attribute("iterations") = "50";
    option.append_attribute("tolerance") = "1e-10";
    option.append_attribute("solver") = "Newton";
    option.append_attribute("jacobian") = "dense";
    option.append_attribute("cone") = "pyramidal";

    xml_node compiler = mujoco.append_child("compiler");
    compiler.append_attribute("angle") = "radian";
    compiler.append_attribute("meshdir") = "resources/meshes/";

    xml_node include_node = mujoco.append_child("include");
    include_node.append_attribute("file")
        = (filesystem::path(PROJECT_ROOT) / "resources" / "includes" / (scene.field + ".xml")).c_str();

    xml_node visual = mujoco.append_child("visual");
    xml_node map = visual.append_child("quality");
    map.append_attribute("shadowsize") = "0";

    include_node = mujoco.append_child("include");
    include_node.append_attribute("file")
        = (filesystem::path(PROJECT_ROOT) / "resources" / "includes" / "ball.xml").c_str();

    for (const string& robotType : robotTypes)
        buildRobotCommon(robotType, mujoco);

    xml_node asset = mujoco.append_child("asset");

    // TODO: This could be parametrized as well. Kinda useless.
    xml_node texture = asset.append_child("texture");
    texture.append_attribute("type") = "skybox";
    texture.append_attribute("builtin") = "gradient";
    texture.append_attribute("rgb1") = "0.3 0.5 0.7";
    texture.append_attribute("rgb2") = "0 0 0";
    texture.append_attribute("width") = "512";
    texture.append_attribute("height") = "512";

    xml_node worldbody = mujoco.append_child("worldbody");
    xml_node actuator = mujoco.append_child("actuator");
    xml_node sensor = mujoco.append_child("sensor");

    xml_node light = worldbody.append_child("light");
    light.append_attribute("ambient") = "1.0 1.0 1.0";
    light.append_attribute("diffuse") = "0.0 0.0 0.0";
    light.append_attribute("specular") = "0.0 0.0 0.0";
    light.append_attribute("pos") = "0 0 100";
    light.append_attribute("dir") = "0 0 -1";

    for (const shared_ptr<Team>& team : scene.teams) {
        for (const shared_ptr<Robot>& robot : team->robots) {
            // TODO use team name to setup jerseys
            buildRobotInstance(robot, worldbody, actuator, sensor);
        }
    }

    ostringstream oss;
    doc.save(oss, "  ");

    return oss.str();
}

void SceneParser::buildRobotCommon(const string& robotType, xml_node& mujoco) {
    filesystem::path commonPath
        = filesystem::path(PROJECT_ROOT) / "resources" / "robots" / robotType / "common.xml";
    if (!filesystem::exists(commonPath)) {
        throw runtime_error("Robot common file does not exist: " + commonPath.string());
    }
    xml_node include_node = mujoco.append_child("include");
    include_node.append_attribute("file") = commonPath.c_str();
}

void SceneParser::prefixSubtree(xml_node& root, const string& robotName) {
    // DFS traversal of the tree, appending the prefix robotName to all names
    stack<xml_node> stack;
    stack.push(root);

    while (!stack.empty()) {
        xml_node node = stack.top();
        stack.pop();

        for (xml_attribute attr : node.attributes()) {
            string current_attr(attr.name());
            // Not the cleanest solution, but tracking all the changed names would require O(n²). I hope this
            // heuristic is general enough.
            if (current_attr == "name" || current_attr == "joint" || current_attr == "objname"
                || current_attr == "site") {
                string original = attr.value();
                if (original.rfind(robotName, 0) != 0) {
                    attr.set_value((robotName + "_" + original).c_str());
                }
            }
        }

        for (xml_node child = node.last_child(); child; child = child.previous_sibling()) {
            stack.push(child);
        }
    }
}

void SceneParser::buildRobotInstance(const shared_ptr<Robot>& robotSpec, xml_node& worldbody,
                                     xml_node& actuator, xml_node& sensor) {
    filesystem::path instancePath
        = filesystem::path(PROJECT_ROOT) / "resources" / "robots" / robotSpec->type / "instance.xml";

    if (!filesystem::exists(instancePath)) {
        throw runtime_error("Robot instance file does not exist: " + instancePath.string());
    }

    xml_document instanceModel;
    if (!instanceModel.load_file(instancePath.c_str())) {
        throw runtime_error("Failed to load robot instance XML: " + instancePath.string());
    }

    xml_node mujoco = instanceModel.child("mujoco");

    xml_node worldbodyModel = mujoco.child("worldbody");
    xml_node sensorModel = mujoco.child("sensor");
    xml_node actuatorModel = mujoco.child("actuator");

    if (!worldbodyModel)
        throw runtime_error("Missing <worldbody> node in <mujoco>.");

    if (!sensorModel)
        throw runtime_error("Missing <sensor> node in <mujoco>.");

    if (!actuatorModel)
        throw runtime_error("Missing <actuator> node in <mujoco>.");

    if (distance(worldbodyModel.begin(), worldbodyModel.end()) != 1)
        throw runtime_error("<worldbody> must have exactly one direct child.");

    xml_node robotNode = *worldbodyModel.begin();

    std::ostringstream posStream;
    posStream << robotSpec->initPosition.x() << " " << robotSpec->initPosition.y() << " "
              << robotSpec->initPosition.z();
    xml_attribute posAttr = robotNode.attribute("pos");
    if (posAttr) {
        posAttr.set_value(posStream.str().c_str());
    } else {
        robotNode.append_attribute("pos") = posStream.str().c_str();
    }

    std::ostringstream oriStream;
    oriStream << robotSpec->initOrientation.x() << " " << robotSpec->initOrientation.y() << " "
              << robotSpec->initOrientation.z();
    xml_attribute eulerAttr = robotNode.attribute("euler");
    if (eulerAttr) {
        eulerAttr.set_value(oriStream.str().c_str());
    } else {
        robotNode.append_attribute("euler") = oriStream.str().c_str();
    }

    prefixSubtree(worldbodyModel, robotSpec->name);
    prefixSubtree(sensorModel, robotSpec->name);
    prefixSubtree(actuatorModel, robotSpec->name);

    for (xml_node child : worldbodyModel.children()) {
        worldbody.append_copy(child);
    }

    for (xml_node child : sensorModel.children()) {
        sensor.append_copy(child);
    }

    for (xml_node child : actuatorModel.children()) {
        actuator.append_copy(child);
    }
}

}  // namespace spqr
