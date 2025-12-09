#include "AppWindow.h"

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <csignal>

#include "Constants.h"
#include "MujocoContext.h"
#include "RobotManager.h"
#include "SceneParser.h"


namespace spqr {

AppWindow::AppWindow(int& argc, char** argv) {
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);

    resize(spqr::initialWindowWidth, spqr::initialWindowHeight);
    setWindowTitle(spqr::appName);

    QWidget* centralWidget = new QWidget;
    mainLayout = new QVBoxLayout;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    viewportContainer = nullptr;

    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* openSceneAction = new QAction("&Open Scene", this);
    fileMenu->addAction(openSceneAction);
    connect(openSceneAction, &QAction::triggered, this, &AppWindow::openScene);

    std::optional<std::string> scenePath;

    if (argc >= 2 && std::string(argv[1]).ends_with(".yaml")) {
        scenePath = argv[1];
    }

    if (scenePath) {
        QString fileArg = QString::fromLocal8Bit(scenePath->c_str());
        loadScene(fileArg);
    }
};

void AppWindow::openScene() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Scene File"), "resources/scenes/",
                                                    tr("YAML Files (*.yaml)"));
    if (!fileName.isEmpty()) {
        loadScene(fileName);
    }
}

void AppWindow::loadScene(const QString& yamlFile) {
    try {
        TeamManager::instance().clear();
        RobotManager::instance().stopCommunicationServer();

        if (sim) {
            sim->stop();
            sim.reset();
        }

        if (viewportContainer) {
            mainLayout->removeWidget(viewportContainer);
            viewportContainer->deleteLater();
            viewportContainer = nullptr;
        }

        SceneParser parser(yamlFile.toStdString());
        std::string xmlScene = parser.buildMuJoCoXml();

        mujContext = std::make_unique<MujocoContext>(xmlScene);
        viewport = std::make_unique<SimulationViewport>(*mujContext);

        RobotManager::instance().startContainers();
        RobotManager::instance().bindMujoco(mujContext.get());

        viewportContainer = QWidget::createWindowContainer(viewport.get());
        mainLayout->addWidget(viewportContainer);

        sim = std::make_unique<SimulationThread>(mujContext->model, mujContext->data);
        sim->start();

        DebugDrawings::init(&(mujContext->scene));

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error loading scene", e.what());
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
