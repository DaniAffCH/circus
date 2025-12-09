#include "CircusApplication.h"

#include <QSurfaceFormat>

#include "curl/curl.h"

namespace spqr {
CircusApplication::CircusApplication(int& argc, char** argv) : QApplication(argc, argv) {
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setSwapInterval(1);
    format.setVersion(2, 0);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRenderableType(QSurfaceFormat::RenderableType::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(format);
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

CircusApplication::~CircusApplication() {
    curl_global_cleanup();
}
}  // namespace spqr
