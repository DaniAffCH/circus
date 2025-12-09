#include <nanobind/nanobind.h>

#include <iostream>

#include "Constants.h"

namespace spqr {

void test() {
    std::cout << appName << std::endl;
}
namespace python {

NB_MODULE(circuspy, m) {
    m.def("test", &spqr::test);
}

}  // namespace python
}  // namespace spqr
