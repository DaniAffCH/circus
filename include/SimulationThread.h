#pragma once

#include <mujoco/mujoco.h>

#include <QThread>

namespace spqr {

class SimulationThread : public QThread {
    Q_OBJECT
   public:
    SimulationThread(const mjModel* model, mjData* data);
    void run() override;
    void stop();

   private:
    const mjModel* model_;
    mjData* data_;
    std::atomic<bool> running_;
};

}  // namespace spqr
