#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>

#include "sensors/Sensor.h"

namespace spqr {

class Pose : public Sensor {
   public:
    Pose(mjModel* mujModel, mjData* mujData, const char* positionName, const char* orientationName)
        : mujModel(mujModel), mujData(mujData) {
        positionId = mj_name2id(mujModel, mjOBJ_SENSOR, positionName);
        positionAdr = mujModel->sensor_adr[positionId];
        positionDim = mujModel->sensor_dim[positionId];

        orientationId = mj_name2id(mujModel, mjOBJ_SENSOR, orientationName);
        orientationAdr = mujModel->sensor_adr[orientationId];
        orientationDim = mujModel->sensor_dim[orientationId];
    }

    void doUpdate() override {
        position = Eigen::Vector3d(Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + positionAdr));
        quatOrientation
            = Eigen::Vector4d(Eigen::Map<const Eigen::Vector4d>(mujData->sensordata + orientationAdr));
        eulerOrientation = quatOrientationToEuler();
        rotationMatrix = quatOrientationToRotationMatrix();
        transformationMatrix = computeTransformationMatrix();
    };

    msgpack::object doSerialize(msgpack::zone& z) override {
        std::vector<double> position_vec = {position(0), position(1), position(2)};

        std::vector<double> quat_orientation_vec
            = {quatOrientation(0), quatOrientation(1), quatOrientation(2), quatOrientation(3)};

        std::vector<double> euler_orientation_vec
            = {eulerOrientation(0), eulerOrientation(1), eulerOrientation(2)};

        std::vector<double> rot_mat_vec = {rotationMatrix(0, 0), rotationMatrix(0, 1), rotationMatrix(0, 2),
                                           rotationMatrix(1, 0), rotationMatrix(1, 1), rotationMatrix(1, 2),
                                           rotationMatrix(2, 0), rotationMatrix(2, 1), rotationMatrix(2, 2)};

        std::vector<double> transformation_matrix_vec
            = {transformationMatrix(0, 0), transformationMatrix(0, 1), transformationMatrix(0, 2),
               transformationMatrix(0, 3), transformationMatrix(1, 0), transformationMatrix(1, 1),
               transformationMatrix(1, 2), transformationMatrix(1, 3), transformationMatrix(2, 0),
               transformationMatrix(2, 1), transformationMatrix(2, 2), transformationMatrix(2, 3),
               transformationMatrix(3, 0), transformationMatrix(3, 1), transformationMatrix(3, 2),
               transformationMatrix(3, 3)};

        std::map<std::string, msgpack::object> pose_data;
        pose_data["position"] = msgpack::object(position_vec, z);
        pose_data["quat_orientation"] = msgpack::object(quat_orientation_vec, z);
        pose_data["euler_orientation"] = msgpack::object(euler_orientation_vec, z);
        pose_data["rotation_matrix"] = msgpack::object(rot_mat_vec, z);
        pose_data["transformation_matrix"] = msgpack::object(transformation_matrix_vec, z);

        return msgpack::object(pose_data, z);
    }

    Eigen::Vector3d getPosition() const {
        return position;
    }
    Eigen::Vector4d getQuatOrientation() const {
        return quatOrientation;
    }
    Eigen::Vector3d getEulerOrientation() const {
        return eulerOrientation;
    }
    Eigen::Matrix3d getRotationMatrix() const {
        return rotationMatrix;
    }
    Eigen::Matrix4d getTransformationMatrix() const {
        return transformationMatrix;
    }

   private:
    Eigen::Vector3d position;          // [x, y, z] : position of the Imu in the world frame
    Eigen::Vector4d quatOrientation;   // [q0, qx, qy, qz] : orientation of the Imu wrt the world frame
    Eigen::Vector3d eulerOrientation;  // [roll, pitch, yaw] : orientation of the Imu wrt the world frame
    Eigen::Matrix3d rotationMatrix;  // [ [R1 R2 R3] [R4 R5 R6] [R7 R8 R9] ] : rotation matrix of the Imu wrt
                                     // the world frame
    Eigen::Matrix4d transformationMatrix;  // 4x4 homogeneous transformation matrix of the Imu wrt the world
                                           // frame

    mjModel* mujModel;
    mjData* mujData;

    int positionId, positionAdr, positionDim;
    int orientationId, orientationAdr, orientationDim;

    Eigen::Vector3d quatOrientationToEuler() {
        Eigen::Quaterniond q(quatOrientation(0), quatOrientation(1), quatOrientation(2), quatOrientation(3));
        Eigen::Vector3d euler = q.toRotationMatrix().eulerAngles(2, 1, 0);
        return Eigen::Vector3d(euler(2), euler(1), euler(0));
    }

    Eigen::Matrix3d quatOrientationToRotationMatrix() {
        Eigen::Quaterniond q(quatOrientation(0), quatOrientation(1), quatOrientation(2), quatOrientation(3));
        return q.toRotationMatrix();
    }

    Eigen::Matrix4d computeTransformationMatrix() {
        Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
        T.block<3, 3>(0, 0) = rotationMatrix;
        T.block<3, 1>(0, 3) = position;
        return T;
    }
};
}  // namespace spqr
