#pragma once
#include <QApplication>

namespace spqr {

class CircusApplication : public QApplication {
   public:
    CircusApplication(int& argc, char** argv);
    ~CircusApplication();
};

}  // namespace spqr
