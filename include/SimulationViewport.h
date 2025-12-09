#pragma once

#include <mujoco/mujoco.h>
#include <qpoint.h>

#include <QOpenGLWindow>
#include <QTimer>
#include <QWheelEvent>

#include "Constants.h"
#include "DebugDrawings.h"
#include "MujocoContext.h"
namespace spqr {

class SimulationViewport : public QOpenGLWindow {
   public:
    SimulationViewport(MujocoContext& mujContext);

   protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

   private:
    QPointF lastMousePosition;
    mjtMouse mouseAction = mjMOUSE_NONE;

    mjModel* model;
    mjData* data;
    mjvCamera* cam;
    mjvOption* opt;
    mjvScene* scene;
    mjrContext* context;
    QTimer* timer;
    mjvPerturb pert;

    /**
     * @brief Selects a body in the simulation based on relative x and y coordinates.
     *
     * This function determines which body in the simulation is located at the given
     * relative screen coordinates (relx, rely). The coordinates are typically normalized
     * between 0 and 1, representing the position within the viewport.
     *
     * @param relx The relative x-coordinate within the viewport (0.0 to 1.0).
     * @param rely The relative y-coordinate within the viewport (0.0 to 1.0).
     * @return The index of the selected body, or -1 if no body is selected.
     */
    int selectBody(float relx, float rely) const;
    int findBodyRoot(int bodyId) const;
    int selectedRobot = -1;

    int width = initialWindowWidth, height = initialWindowHeight;
    int logicalWidth = initialWindowWidth, logicalHeight = initialWindowHeight;
};

}  // namespace spqr
