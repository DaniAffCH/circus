#include "MujocoContext.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>

namespace spqr {
MujocoContext::MujocoContext(const std::string& xmlString) {
    char error[1024] = {0};
    std::filesystem::current_path(PROJECT_ROOT);

    std::unique_ptr<mjSpec, std::function<void(mjSpec*)> > spec(
        mj_parseXMLString(xmlString.c_str(), nullptr, error, sizeof(error)),
        [](mjSpec* s) { mj_deleteSpec(s); });

    if (!spec) {
        throw std::runtime_error(std::string("Failed to parse the generated XML. ") + error);
    }

    model = mj_compile(spec.get(), nullptr);
    if (!model) {
        throw std::runtime_error(std::string("Failed to compile mujoco specs. ") + mjs_getError(spec.get()));
    }

    data = mj_makeData(model);

    mjv_defaultOption(&opt);
    mjv_defaultCamera(&cam);
    mjv_makeScene(model, &scene, 10000);
}

MujocoContext::~MujocoContext() {
    if (data)
        mj_deleteData(data);
    if (model)
        mj_deleteModel(model);
    mjv_freeScene(&scene);
}

MujocoContext& MujocoContext::operator=(MujocoContext&& other) noexcept {
    if (this != &other) {
        if (data)
            mj_deleteData(data);
        if (model)
            mj_deleteModel(model);
        mjv_freeScene(&scene);

        model = other.model;
        data = other.data;
        cam = other.cam;
        opt = other.opt;
        scene = other.scene;

        other.model = nullptr;
        other.data = nullptr;
    }
    return *this;
}
}  // namespace spqr
