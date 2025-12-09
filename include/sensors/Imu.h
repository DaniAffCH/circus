#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>

#include "sensors/Sensor.h"

namespace spqr {

class Imu : public Sensor {
   public:
    Imu(mjModel* mujModel, mjData* mujData, const char* linearAccelerationName,
        const char* angularVelocityName)
        : mujModel(mujModel), mujData(mujData) {
        accId = mj_name2id(mujModel, mjOBJ_SENSOR, linearAccelerationName);
        accAdr = mujModel->sensor_adr[accId];
        accDim = mujModel->sensor_dim[accId];

        gyroId = mj_name2id(mujModel, mjOBJ_SENSOR, angularVelocityName);
        gyroAdr = mujModel->sensor_adr[gyroId];
        gyroDim = mujModel->sensor_dim[gyroId];
    }

    void doUpdate() override {
        linearAcceleration = Eigen::Vector3d(Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + accAdr));
        angularVelocity = Eigen::Vector3d(Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + gyroAdr));
    };

    msgpack::object doSerialize(msgpack::zone& z) override {
        std::vector<double> linear_acc_vec
            = {linearAcceleration(0), linearAcceleration(1), linearAcceleration(2)};
        std::vector<double> angular_vel_vec = {angularVelocity(0), angularVelocity(1), angularVelocity(2)};

        std::map<std::string, msgpack::object> imu_data;
        imu_data["linear_acceleration"] = msgpack::object(linear_acc_vec, z);
        imu_data["angular_velocity"] = msgpack::object(angular_vel_vec, z);
        return msgpack::object(imu_data, z);
    }

    Eigen::Vector3d getLinearAcceleration() const {
        return linearAcceleration;
    }
    Eigen::Vector3d getAngularVelocity() const {
        return angularVelocity;
    }

   private:
    Eigen::Vector3d linearAcceleration;  // [ax, ay, az] : linear acceleration expressed in the local frame of
                                         // the
    Eigen::Vector3d angularVelocity;  // [wx, wy, wz] : angular velocity expressed in the local frame of the

    mjModel* mujModel;
    mjData* mujData;

    int accId, accAdr, accDim;
    int gyroId, gyroAdr, gyroDim;
};
}  // namespace spqr
