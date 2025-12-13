#include "AppWindow.h"

#include <qdebug.h>

#include <QDebug>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QWindow>
#include <QtQml>
#include <csignal>
#include <fstream>

#include "MujocoContext.h"
#include "RobotManager.h"
#include "RobotQmlWrapper.h"
#include "SceneParser.h"
#include "Team.h"
#include "ToolCellWrapper.h"

namespace spqr {

QVariantList AppWindow::getTeamsForQml() const {
    QVariantList teamsList;

    auto teams = TeamManager::instance().getTeams();

    for (const auto& team : teams) {
        QVariantMap teamMap;
        teamMap["name"] = QString::fromStdString(team->name);

        QVariantList robotsList;
        for (const auto& robot : team->robots) {
            auto robotWrapper = new RobotQmlWrapper(robot, const_cast<AppWindow*>(this));
            const_cast<AppWindow*>(this)->robotWrappers_.append(robotWrapper);
            robotsList.append(QVariant::fromValue(robotWrapper));
        }
        teamMap["robots"] = robotsList;
        teamsList.append(teamMap);
    }

    return teamsList;
}

QVariantMap AppWindow::getGuiConfig() const {
    return currentGuiConfig_;
}

void AppWindow::updateRobotData() {
    // Call update() on all robot wrappers to emit signals
    for (QObject* obj : robotWrappers_) {
        if (auto wrapper = qobject_cast<RobotQmlWrapper*>(obj)) {
            wrapper->update();
        }
    }
}

AppWindow::AppWindow(int& argc, char** argv) {
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);

    // Register custom QML types
    qmlRegisterType<ToolCellWrapper>("Circus", 1, 0, "ToolCellWrapper");

    // Create QML engine ONCE
    qmlEngine = std::make_unique<QQmlApplicationEngine>();
    qmlEngine->rootContext()->setContextProperty("appWindow", this);

    const QUrl qrcUrl = QUrl(QStringLiteral("resources/qml/main.qml"));

    // Connect to objectCreated to detect if QML failed to load
    QObject::connect(qmlEngine.get(), &QQmlApplicationEngine::objectCreated,
                     [](QObject* obj, const QUrl& objUrl) {
                         if (!obj) {
                             qCritical() << "Failed to load QML:" << objUrl;
                             QCoreApplication::exit(-1);
                         }
                     });

    qmlEngine->load(qrcUrl);

    // Load scene from command line argument if provided
    std::optional<std::string> scenePath;
    if (argc >= 2 && std::string(argv[1]).ends_with(".yaml")) {
        scenePath = argv[1];
    }
    if (scenePath) {
        QString fileArg = QString::fromLocal8Bit(scenePath->c_str());
        QTimer::singleShot(100, [this, fileArg]() { loadScene(fileArg); });
    }
};

void AppWindow::loadScene(const QString& yamlFile) {
    try {
        qDebug() << "Loading scene:" << yamlFile;

        // Store the current scene path
        currentScenePath_ = yamlFile;

        qDebug() << "1. Clearing team manager...";
        TeamManager::instance().clear();

        qDeleteAll(robotWrappers_);
        robotWrappers_.clear();

        qDebug() << "2. Stopping communication server...";
        RobotManager::instance().stopCommunicationServer();

        if (sim) {
            qDebug() << "3. Stopping simulation...";
            sim->stop();
            sim.reset();
        }

        if (viewportContainer) {
            qDebug() << "4. Cleaning up old viewport...";
            viewportContainer->deleteLater();
            viewportContainer = nullptr;
        }

        if (viewport) {
            qDebug() << "4b. Cleaning up viewport window...";
            viewport.reset();
        }

        qDebug() << "5. Parsing scene...";
        SceneParser parser(yamlFile.toStdString());
        std::string xmlScene = parser.buildMuJoCoXml();

        const auto& sceneInfo = parser.getSceneInfo();
        currentGuiConfig_["numRows"] = sceneInfo.guiConfig.rows;
        currentGuiConfig_["numColumns"] = sceneInfo.guiConfig.columns;

        // Expose cell data as QML list of objects
        QVariantList cellDataList;
        for (const auto& cellData : sceneInfo.guiConfig.cellData) {
            QVariantMap cellMap;
            cellMap["row"] = cellData.row;
            cellMap["column"] = cellData.column;
            cellMap["stream"] = QString::fromStdString(cellData.stream);
            cellDataList.append(cellMap);
        }
        currentGuiConfig_["cellData"] = cellDataList;

        qDebug() << "GUI Config from scene:" << sceneInfo.guiConfig.rows << "rows,"
                 << sceneInfo.guiConfig.columns << "columns"
                 << "with" << sceneInfo.guiConfig.cellData.size() << "cell data entries";

        qDebug() << "6. Creating MuJoCo context...";
        mujContext = std::make_unique<MujocoContext>(xmlScene);

        qDebug() << "7. Creating viewport...";
        viewport = std::make_unique<SimulationViewport>(*mujContext);

        qDebug() << "8. Starting containers...";
        RobotManager::instance().startContainers();

        qDebug() << "9. Binding MuJoCo...";
        RobotManager::instance().bindMujoco(mujContext.get());

        qDebug() << "10. Embedding viewport in QML...";

        if (qmlEngine->rootObjects().isEmpty()) {
            qWarning() << "No root QML objects found!";
            return;
        }

        QObject* rootObject = qmlEngine->rootObjects().first();
        QQuickItem* qmlContainer = rootObject->findChild<QQuickItem*>("viewportContainer");

        if (qmlContainer) {
            QQuickWindow* quickWindow = qmlContainer->window();

            if (quickWindow) {
                viewport->setTransientParent(quickWindow);
                viewport->setFlags(Qt::Widget);
                viewportContainer = QWidget::createWindowContainer(viewport.get());
                viewportContainer->setParent(nullptr);
                viewportContainer->setWindowFlags(Qt::Widget);
                WId viewportWinId = viewportContainer->winId();
                WId quickWinId = quickWindow->winId();

// On X11, we can directly set the parent using xcb
#ifdef Q_OS_LINUX
                viewportContainer->windowHandle()->setParent(quickWindow);
#endif

                auto updateGeometry = [qmlContainer, quickWindow, this]() {
                    QPointF scenePos = qmlContainer->mapToScene(QPointF(0, 0));
                    QPoint globalPos = quickWindow->mapToGlobal(scenePos.toPoint());

                    this->viewportContainer->setGeometry(scenePos.x(), scenePos.y(), qmlContainer->width(),
                                                         qmlContainer->height());
                };

                updateGeometry();

                QObject::connect(qmlContainer, &QQuickItem::widthChanged, updateGeometry);
                QObject::connect(qmlContainer, &QQuickItem::heightChanged, updateGeometry);
                QObject::connect(qmlContainer, &QQuickItem::xChanged, updateGeometry);
                QObject::connect(qmlContainer, &QQuickItem::yChanged, updateGeometry);

                viewportContainer->show();
                viewportContainer->raise();

                qDebug() << "Viewport embedded successfully";
            } else {
                qWarning() << "Could not get QQuickWindow";
            }
        } else {
            qWarning() << "Could not find viewportContainer in QML";
        }

        qDebug() << "12. Starting simulation thread...";
        sim = std::make_unique<SimulationThread>(mujContext->model, mujContext->data);

        // Connect simulation step to update robot data
        connect(sim.get(), &SimulationThread::stepCompleted, this, &AppWindow::updateRobotData,
                Qt::QueuedConnection);

        sim->start();

        qDebug() << "13. Emitting teams changed signal...";
        emit teamsChanged();

        qDebug() << "14. Emitting gui config changed signal...";
        emit guiConfigChanged();

        qDebug() << "Scene loaded successfully";
    } catch (const std::exception& e) {
        qWarning() << "Error loading scene:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error loading scene";
    }
}

QString AppWindow::getCurrentScenePath() const {
    return currentScenePath_;
}

void AppWindow::saveGuiConfig(const QString& yamlFile, const QVariantList& cellData, int numRows, int numColumns) {
    try {
        qDebug() << "Saving GUI config to:" << yamlFile;

        YAML::Node root;

        // Check if target file exists
        std::ifstream fin(yamlFile.toStdString());
        bool fileExists = fin.good();
        fin.close();

        if (fileExists) {
            // File exists, load and update it
            root = YAML::LoadFile(yamlFile.toStdString());
        } else {
            // File doesn't exist, copy from current scene
            if (!currentScenePath_.isEmpty()) {
                qDebug() << "Copying scene structure from:" << currentScenePath_;
                // TODO: When the possibility to add new robots is implemented, modify this to
                // get robot data (position, orientation, type, number) from the running simulation
                // and update the new YAML file with the current robot states instead of copying
                // from the source file
                root = YAML::LoadFile(currentScenePath_.toStdString());
            } else {
                // No current scene, create minimal structure
                qDebug() << "No current scene, creating minimal YAML structure";
                root["field"] = "fieldRCAP";
                root["ball"]["position"] = YAML::Node(YAML::NodeType::Sequence);
                root["ball"]["position"].push_back(0.0);
                root["ball"]["position"].push_back(0.0);
                root["ball"]["position"].push_back(0.12);
                root["teams"] = YAML::Node(YAML::NodeType::Map);
            }
        }

        // Update gui_config section (always update regardless of whether file existed)
        if (!root["gui_config"]) {
            root["gui_config"] = YAML::Node(YAML::NodeType::Sequence);
        }

        // Ensure gui_config has at least 2 elements
        while (root["gui_config"].size() < 2) {
            root["gui_config"].push_back(YAML::Node(YAML::NodeType::Map));
        }

        // Update tools_panel dimensions in the first element
        YAML::Node toolsPanelDims(YAML::NodeType::Sequence);
        toolsPanelDims.push_back(numRows);
        toolsPanelDims.push_back(numColumns);
        root["gui_config"][0]["tools_panel"] = toolsPanelDims;

        // Update cell_data in the second element
        YAML::Node cellDataNode(YAML::NodeType::Sequence);
        for (const QVariant& cellVariant : cellData) {
            QVariantMap cellMap = cellVariant.toMap();

            YAML::Node cellNode;
            YAML::Node cellCoords(YAML::NodeType::Sequence);
            cellCoords.push_back(cellMap["row"].toInt());
            cellCoords.push_back(cellMap["column"].toInt());

            cellNode["cell"] = cellCoords;
            cellNode["stream"] = cellMap["stream"].toString().toStdString();

            cellDataNode.push_back(cellNode);
        }

        root["gui_config"][1]["cell_data"] = cellDataNode;

        // Write back to file
        std::ofstream fout(yamlFile.toStdString());
        fout << root;
        fout.close();

        qDebug() << "GUI config saved successfully";
    } catch (const std::exception& e) {
        qWarning() << "Error saving GUI config:" << e.what();
    }
}

void AppWindow::signalHandler(int signal) {
    TeamManager::instance().clear();
    RobotManager::instance().stopCommunicationServer();

    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

AppWindow::~AppWindow() {
    if (sim != nullptr && sim->isRunning())
        sim->stop();
    TeamManager::instance().clear();
    RobotManager::instance().stopCommunicationServer();
}
}  // namespace spqr
