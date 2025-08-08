#pragma once
#include <QMainWindow>
#include <mujoco/mujoco.h>
#include "SimulationViewport.h"
#include "MujocoContext.h"
#include "SimulationThread.h"
#include <memory>
#include <QVBoxLayout>
namespace spqr {

class AppWindow : public QMainWindow
{
public:
    AppWindow(int& argc, char** argv);
    ~AppWindow();

private:
    void loadScene(const QString& xml);
    void openScene();
    
    QVBoxLayout* mainLayout;
    QWidget* viewportContainer;

    std::unique_ptr<MujocoContext> mujContext;
    std::unique_ptr<SimulationViewport> viewport;
    std::unique_ptr<SimulationThread> sim;
};

} 