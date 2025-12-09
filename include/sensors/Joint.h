#pragma once

#include <mujoco/mujoco.h>

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "sensors/Sensor.h"

namespace spqr {

enum class JointValue {
    HEAD_YAW,
    HEAD_PITCH,
    SHOULDER_LEFT_PITCH,
    SHOULDER_LEFT_ROLL,
    ELBOW_LEFT_PITCH,
    ELBOW_LEFT_YAW,
    SHOULDER_RIGHT_PITCH,
    SHOULDER_RIGHT_ROLL,
    ELBOW_RIGHT_PITCH,
    ELBOW_RIGHT_YAW,
    WAIST,
    HIP_LEFT_PITCH,
    HIP_LEFT_ROLL,
    HIP_LEFT_YAW,
    KNEE_LEFT_PITCH,
    ANKLE_LEFT_PITCH,
    ANKLE_LEFT_ROLL,
    HIP_RIGHT_PITCH,
    HIP_RIGHT_ROLL,
    HIP_RIGHT_YAW,
    KNEE_RIGHT_PITCH,
    ANKLE_RIGHT_PITCH,
    ANKLE_RIGHT_ROLL,
};

class Joints : public Sensor {
   public:
    Joints(mjModel* mujModel, mjData* mujData, std::map<JointValue, std::string> map)
        : mujModel(mujModel), mujData(mujData) {
        size_t idx = 0;
        order.reserve(map.size());
        for (auto& [jv, joint_name] : map) {
            int jointId = mj_name2id(mujModel, mjOBJ_JOINT, joint_name.c_str());
            if (jointId == -1)
                throw std::runtime_error("Joint not found: " + joint_name);

            index_of[jv] = idx;
            joint_ids.push_back(jointId);
            order.push_back(jv);

            int actuator_id = -1;
            for (int act_id = 0; act_id < mujModel->nu; ++act_id) {
                if (mujModel->actuator_trntype[act_id] == mjTRN_JOINT
                    && mujModel->actuator_trnid[2 * act_id] == jointId) {
                    actuator_id = act_id;
                    break;
                }
            }
            actuator_ids.push_back(actuator_id);
            ++idx;
        }

        size = idx;
        position.resize(size);
        velocity.resize(size);
        acceleration.resize(size);
        torque.resize(size);
    }

    void doUpdate() override {
        for (size_t i = 0; i < size; ++i) {
            int jid = joint_ids[i];
            position[i] = mujData->qpos[mujModel->jnt_qposadr[jid]];
            velocity[i] = mujData->qvel[mujModel->jnt_dofadr[jid]];
            acceleration[i] = mujData->qacc[mujModel->jnt_dofadr[jid]];
            torque[i] = mujData->qfrc_actuator[mujModel->jnt_dofadr[jid]];
        }
    }

    void set_position(const std::unordered_map<JointValue, mjtNum>& values) {
        for (const auto& [joint, val] : values) {
            auto it = index_of.find(joint);
            if (it == index_of.end())
                continue;
            size_t i = it->second;
            int jid = joint_ids[i];
            mujData->qpos[mujModel->jnt_qposadr[jid]] = val;
        }
        mj_forward(mujModel, mujData);
    }

    void set_torque(const std::unordered_map<JointValue, mjtNum>& values) {
        for (const auto& [joint, val] : values) {
            auto it = index_of.find(joint);
            if (it == index_of.end())
                continue;
            size_t i = it->second;
            int act_id = actuator_ids[i];
            if (act_id < 0)
                continue;
            mujData->ctrl[act_id] = std::clamp(val, mujModel->actuator_ctrlrange[2 * act_id],
                                               mujModel->actuator_ctrlrange[2 * act_id + 1]);
        }
    }

    msgpack::object doSerialize(msgpack::zone& z) override {
        std::vector<mjtNum> pos(position.begin(), position.begin() + size);
        std::vector<mjtNum> vel(velocity.begin(), velocity.begin() + size);
        std::vector<mjtNum> acc(acceleration.begin(), acceleration.begin() + size);
        std::vector<mjtNum> tor(torque.begin(), torque.begin() + size);

        std::map<std::string, msgpack::object> data;
        data["position"] = msgpack::object(pos, z);
        data["velocity"] = msgpack::object(vel, z);
        data["acceleration"] = msgpack::object(acc, z);
        data["torque"] = msgpack::object(tor, z);

        return msgpack::object(data, z);
    }

   private:
    mjModel* mujModel;
    mjData* mujData;
    size_t size{0};

    std::vector<JointValue> order;
    std::unordered_map<JointValue, size_t> index_of;
    std::vector<int> joint_ids, actuator_ids;
    std::vector<mjtNum> position, velocity, acceleration, torque;
};

}  // namespace spqr
