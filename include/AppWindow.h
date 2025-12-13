#pragma once
#include <mujoco/mujoco.h>
#include <qqmlapplicationengine.h>
#include <qtmetamacros.h>

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "MujocoContext.h"
#include "SimulationThread.h"
#include "SimulationViewport.h"
namespace spqr {

class AppWindow : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString projectRoot READ projectRoot CONSTANT)
    Q_PROPERTY(QVariantList teams READ getTeamsForQml NOTIFY teamsChanged)
    Q_PROPERTY(QVariantMap guiConfig READ getGuiConfig NOTIFY guiConfigChanged)

   public:
    AppWindow(int& argc, char** argv);
    ~AppWindow();

    Q_INVOKABLE void loadScene(const QString& yamlFile);
    Q_INVOKABLE void updateRobotData();
    Q_INVOKABLE void saveGuiConfig(const QString& yamlFile, const QVariantList& cellData, int numRows, int numColumns);
    Q_INVOKABLE QString getCurrentScenePath() const;

    QString projectRoot() const {
        return QString::fromStdString(PROJECT_ROOT);
    }
    QVariantList getTeamsForQml() const;
    QVariantMap getGuiConfig() const;

   signals:
    void teamsChanged();
    void guiConfigChanged();

   private:
    static void signalHandler(int signal);

    std::unique_ptr<QQmlApplicationEngine> qmlEngine;

    QWidget* viewportContainer = nullptr;

    std::unique_ptr<MujocoContext> mujContext;
    std::unique_ptr<SimulationViewport> viewport;
    std::unique_ptr<SimulationThread> sim;

    QList<QObject*> robotWrappers_;
    QVariantMap currentGuiConfig_;
    QString currentScenePath_;
};

}  // namespace spqr
