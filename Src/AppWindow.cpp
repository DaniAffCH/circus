#include "AppWindow.h"
#include "MujocoContext.h"
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <memory>
#include "Constants.h"
#include "SceneParser.h"
namespace spqr {

    AppWindow::AppWindow(int& argc, char** argv) {
        
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

        if (argc > 1) {
            QString fileArg = QString::fromLocal8Bit(argv[1]);
            loadScene(fileArg);
        }
    };

    void AppWindow::openScene() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open Scene File"), "", tr("YAML Files (*.yaml)"));
        if (!fileName.isEmpty()) {
            loadScene(fileName);
        }
    }

    void AppWindow::loadScene(const QString& xml){
        try{
            if(sim){
                sim->stop();
                sim.reset();
            }

            if(viewportContainer){
                mainLayout->removeWidget(viewportContainer);
                viewportContainer->deleteLater();
                viewportContainer = nullptr;
            }

            SceneParser parser(xml.toStdString());
            std::string xmlScene = parser.buildMuJoCoXml();

            mujContext = std::make_unique<MujocoContext>(xmlScene);
            viewport = std::make_unique<SimulationViewport>(*mujContext);

            viewportContainer = QWidget::createWindowContainer(viewport.get());
            mainLayout->addWidget(viewportContainer);

            sim = std::make_unique<SimulationThread>(mujContext->model, mujContext->data);
            sim->start();
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error loading scene", e.what());
        }
    }

    AppWindow::~AppWindow(){
        if(sim != nullptr && sim->isRunning())
            sim->stop();
    }
}
