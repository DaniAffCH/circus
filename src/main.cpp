
#include <QSurfaceFormat>

#include "AppWindow.h"
#include "CircusApplication.h"
#include "Constants.h"

int main(int argc, char** argv) {
    spqr::CircusApplication app(argc, argv);
    app.setApplicationName(spqr::appName);

    spqr::AppWindow window(argc, argv);
    window.show();

    return app.exec();
}
