#pragma once
#include <mujoco/mujoco.h>

#include <QMainWindow>
#include <QVBoxLayout>
#include <memory>

#include "DebugDrawings.h"
#include "MujocoContext.h"
#include "SimulationThread.h"
#include "SimulationViewport.h"

namespace spqr {

class AppWindow : public QMainWindow {
   public:
    AppWindow(int& argc, char** argv);
    ~AppWindow();

   private:
    void loadScene(const QString& xml);
    void openScene();

    static void signalHandler(int signal);

    QVBoxLayout* mainLayout;
    QWidget* viewportContainer;

    std::unique_ptr<MujocoContext> mujContext;
    std::unique_ptr<SimulationViewport> viewport;
    std::unique_ptr<SimulationThread> sim;

    DebugDrawings* debugDrawings;
};

}  // namespace spqr
