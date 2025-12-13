#pragma once

#include <QObject>
#include <QVariantMap>
#include <QString>
#include <QVariantList>
#include <memory>

#include "RobotQmlWrapper.h"

namespace spqr {

class ToolCellWrapper : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap data READ data NOTIFY dataChanged)
    Q_PROPERTY(QString streamType READ streamType CONSTANT)
    Q_PROPERTY(bool hasData READ hasData NOTIFY hasDataChanged)

   public:
    ToolCellWrapper(QObject* parent = nullptr) : QObject(parent) {}

    // Set the stream name - automatically finds and connects to the correct robot
    Q_INVOKABLE void setStream(const QString& streamName, const QVariantList& teams) {
        if (robot_) {
            disconnect(robot_, nullptr, this, nullptr);
            robot_ = nullptr;
        }

        streamName_ = streamName;
        updateStreamType();

        // Parse stream name and find the robot
        robot_ = findRobotFromStreamName(streamName, teams);

        if (robot_) {
            // Connect to robot's update signals
            connect(robot_, &RobotQmlWrapper::imuChanged, this, &ToolCellWrapper::updateData);
            connect(robot_, &RobotQmlWrapper::poseChanged, this, &ToolCellWrapper::updateData);
        }

        updateData();
    }

    // Legacy method for backward compatibility
    void setRobotAndStream(RobotQmlWrapper* robot, const QString& streamName) {
        if (robot_) {
            disconnect(robot_, nullptr, this, nullptr);
        }

        robot_ = robot;
        streamName_ = streamName;
        updateStreamType();

        if (robot_) {
            // Connect to robot's update signals
            connect(robot_, &RobotQmlWrapper::imuChanged, this, &ToolCellWrapper::updateData);
            connect(robot_, &RobotQmlWrapper::poseChanged, this, &ToolCellWrapper::updateData);
        }

        updateData();
    }

    QVariantMap data() const {
        return currentData_;
    }

    QString streamType() const {
        return streamType_;
    }

    bool hasData() const {
        return hasData_;
    }

   public slots:
    // Update data from the robot
    void updateData() {
        if (!robot_ || streamName_.isEmpty()) {
            if (hasData_) {
                hasData_ = false;
                currentData_.clear();
                emit hasDataChanged();
                emit dataChanged();
            }
            return;
        }

        QVariantMap newData;
        bool found = false;

        if (streamName_.contains("IMU Linear Acceleration")) {
            QVariantMap imuData = robot_->imu();
            if (imuData.contains("linearAcceleration")) {
                QVariantList acc = imuData["linearAcceleration"].toList();
                if (acc.size() >= 3) {
                    newData["x"] = acc[0];
                    newData["y"] = acc[1];
                    newData["z"] = acc[2];
                    found = true;
                }
            }
        } else if (streamName_.contains("IMU Angular Velocity")) {
            QVariantMap imuData = robot_->imu();
            if (imuData.contains("angularVelocity")) {
                QVariantList vel = imuData["angularVelocity"].toList();
                if (vel.size() >= 3) {
                    newData["x"] = vel[0];
                    newData["y"] = vel[1];
                    newData["z"] = vel[2];
                    found = true;
                }
            }
        } else if (streamName_.contains("Pose Position")) {
            QVariantMap poseData = robot_->pose();
            if (poseData.contains("position")) {
                QVariantList pos = poseData["position"].toList();
                if (pos.size() >= 3) {
                    newData["x"] = pos[0];
                    newData["y"] = pos[1];
                    newData["z"] = pos[2];
                    found = true;
                }
            }
        } else if (streamName_.contains("Pose Orientation")) {
            QVariantMap poseData = robot_->pose();
            if (poseData.contains("orientation")) {
                QVariantList ori = poseData["orientation"].toList();
                if (ori.size() >= 3) {
                    newData["x"] = ori[0];
                    newData["y"] = ori[1];
                    newData["z"] = ori[2];
                    found = true;
                }
            }
        }

        bool dataChanged = (currentData_ != newData);
        bool hasDataChanged = (hasData_ != found);

        if (found) {
            currentData_ = newData;
            hasData_ = true;
        } else {
            currentData_.clear();
            hasData_ = false;
        }

        if (hasDataChanged) {
            emit this->hasDataChanged();
        }
        if (dataChanged) {
            emit this->dataChanged();
        }
    }

   signals:
    void dataChanged();
    void hasDataChanged();

   private:
    // Parse stream name and find the corresponding robot
    RobotQmlWrapper* findRobotFromStreamName(const QString& streamName, const QVariantList& teams) {
        if (streamName.isEmpty() || teams.isEmpty()) {
            return nullptr;
        }

        // Stream name format: "Team X - RobotName (RY) - DataType"
        // Parse to find team and robot
        for (int teamIdx = 0; teamIdx < teams.size(); ++teamIdx) {
            QVariantMap teamMap = teams[teamIdx].toMap();
            QString teamPrefix = QString("Team %1").arg(teamIdx + 1);

            if (!streamName.startsWith(teamPrefix)) {
                continue;
            }

            QVariantList robots = teamMap["robots"].toList();
            for (const QVariant& robotVariant : robots) {
                RobotQmlWrapper* robot = robotVariant.value<RobotQmlWrapper*>();
                if (!robot) continue;

                QString robotPrefix = QString("%1 - %2 (R%3)")
                                          .arg(teamPrefix)
                                          .arg(robot->name())
                                          .arg(robot->number());

                if (streamName.startsWith(robotPrefix)) {
                    return robot;
                }
            }
        }

        return nullptr;
    }

    void updateStreamType() {
        if (streamName_.contains("IMU")) {
            streamType_ = "imu";
        } else if (streamName_.contains("Pose")) {
            streamType_ = "pose";
        } else {
            streamType_ = "unknown";
        }
    }

    RobotQmlWrapper* robot_ = nullptr;
    QString streamName_;
    QString streamType_;
    QVariantMap currentData_;
    bool hasData_ = false;
};

}  // namespace spqr
