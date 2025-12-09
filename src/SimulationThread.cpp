#include "SimulationThread.h"

#include <stdexcept>

#include "RobotManager.h"

namespace spqr {

SimulationThread::SimulationThread(const mjModel* model, mjData* data)
    : model_(model), data_(data), running_(true) {}

void SimulationThread::run() {
    if (!model_)
        throw std::runtime_error("Cannot start simulation without mujoco model");

    double sim_dt = model_->opt.timestep;

    using clock = std::chrono::steady_clock;
    auto next_step_time = clock::now();
    while (running_) {
        mj_step(model_, data_);
        RobotManager::instance().update();

        next_step_time += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(sim_dt));
        std::this_thread::sleep_until(next_step_time);

        if (clock::now() > next_step_time)
            next_step_time = clock::now();
    }
}

void SimulationThread::stop() {
    running_ = false;
    wait();
}

}  // namespace spqr
