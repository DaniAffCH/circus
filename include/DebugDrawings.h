#pragma once

#include <mujoco/mujoco.h>

#include <QColor>
#include <algorithm>
#include <iostream>
#include <map>
#include <msgpack.hpp>

namespace spqr {
class DebugDrawings {
   public:
    static void init(mjvScene* s) noexcept;
    static void drawDebugDrawings();

    static void processDebugMessage(std::map<std::string, msgpack::object>);
    static void removeGeom(const std::string& idLocal);

    // ---------------- regular geom types (mjtGeom)
    /**
     * @brief drawCircle: initialize a mjGEOM_CIRCLE in mujoco processing a predefined message from the socker
     * with the framework
     * @param idLocal: identifier of the debug drawing
     * @param center: position of the center of the geom
     * @param radius: radius of the circle
     * @param color: RGBA color of the geom
     */
    static void drawCircle(const std::map<std::string, msgpack::object>& data_map);

    /**
     * @brief drawSphere: initialize a mjGEOM_SPHERE in mujoco processing a predefined message from the socker
     * with the framework
     * @param idLocal: identifier of the debug drawing
     * @param center: position of the center of the geom
     * @param radius: radius of the sphere
     * @param color: RGBA color of the geom
     */
    static void drawSphere(const std::map<std::string, msgpack::object>& data_map);

    /**
     * @brief drawCylinder: initialize a mjGEOM_SPHERE in mujoco processing a predefined message from the
     * socker with the framework
     * @param idLocal: identifier of the debug drawing
     * @param center: position of the center of the geom
     * @param radius: radius of the cylinder
     * @param length: half length of the cylinder
     * @param color: RGBA color of the geom
     */
    static void drawCylinder(const std::map<std::string, msgpack::object>& data_map);

    // ---------------- rendering-only geom types (mjtGeom)

    /**
     * @brief drawArrow: initialize a mjGEOM_ARROW in mujoco processing a predefined message from the socker
     * with the framework
     * @param idLocal: identifier of the debug drawing
     * @param start: position of the start point of the arrow
     * @param end: position of the end point of the arrow
     * @param thickness: thickness of the arrow
     * @param color: RGBA color of the geom
     */
    static void drawArrow(const std::map<std::string, msgpack::object>& data_map);

    /**
     * @brief drawLine: initialize a mjGEOM_LINE in mujoco processing a predefined message from the socker
     * with the framework
     * @param idLocal: identifier of the debug drawing
     * @param start: position of the start point of the line
     * @param end: position of the end point of the line
     * @param thickness: thickness of the line
     * @param color: RGBA color of the geom
     */
    static void drawLine(const std::map<std::string, msgpack::object>& data_map);

   private:
    enum class drawGeomType {
        Sphere,
        Cylinder,
        Circle,
        Arrow,
        Line,

        COUNT,  // keep this last
    };

    static const char* toString(drawGeomType t) {
        switch (t) {
            case drawGeomType::Sphere:
                return "Sphere";
            case drawGeomType::Cylinder:
                return "Cylinder";
            case drawGeomType::Circle:
                return "Circle";
            case drawGeomType::Arrow:
                return "Arrow";
            case drawGeomType::Line:
                return "Line";
        }
        return "Unknown";
    }

    constexpr bool isRegularGeom(drawGeomType t) {
        switch (t) {
            case drawGeomType::Sphere:
            case drawGeomType::Cylinder:
            case drawGeomType::Circle:
                return true;
            default:
                return false;
        }
    }

    struct GeomData {
        mjvGeom geom;
        drawGeomType customType;
    };

    inline static mjvScene* ptrDebugDrawingsScene;  // pointer to the mujoco scene where to draw the debug
                                                    // drawings
    inline static std::map<mjString, GeomData> mapIdGeom;

    /**
     * @brief drawRegularGeom: draw a 'regular'-defined geometric primitive
     * @param idLocal: identifier of the debug drawing
     * @param mujocoType: standard MuJoCo geometric type (mjtGeom)
     * @param customType: custom geometric type used to select the appropriate mjvGeom
     * @param size: size of the geometric primitive
     * @param pos: position of the geometric primitive
     * @param color: color of the primitive (RGBA), can use QColorConstants.
     */
    static void drawRegularGeom(mjString idLocal, mjtGeom mujocoType, drawGeomType customType,
                                const double size[3], const double pos[3], QColor color);

    /**
     * @brief drawRenderOnlyGeom: draw a 'rendering-only'-defined geometric primitive
     * @param idLocal: identifier of the debug drawing
     * @param mujocoType: standard MuJoCo geometric type (mjtGeom)
     * @param customType: custom geometric type used to select the appropriate mjvGeom
     * @param size: size of the geometric primitive
     * @param start: start position of the geometric primitive
     * @param end: end position of the geometric primitive
     * @param width: width of the geometric primitive
     * @param color: RGBA color of the geometric primitive, can be a QColorConstants
     */
    static void drawRenderOnlyGeom(mjString idLocal, mjtGeom mujocoType, drawGeomType customType,
                                   const double size[3], const double start[3], const double end[3],
                                   const double width, QColor color);

    /**
     * @brief check is the mjv_Geom already exist. If true return the right mjvGeom into mjvScene->geoms
     */

    static mjvGeom* isGeomExist(mjString idLocal, drawGeomType type);

    /**
     * @brief move the mjvGeom object
     */
    static void moveGeom(mjvGeom* geom, const double center[3], QColor color);
    static void moveGeom(mjvGeom* geom, const double start[3], const double end[3], QColor color);
};

}  // namespace spqr
