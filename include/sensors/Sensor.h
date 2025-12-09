#pragma once

#include <msgpack.hpp>
#include <mutex>
#include <shared_mutex>

class Sensor {
   public:
    virtual ~Sensor() = default;

    virtual void update() final {
        std::unique_lock lock(mtx_);
        doUpdate();
    }

    virtual msgpack::object serialize(msgpack::zone& z) final {
        std::shared_lock lock(mtx_);
        return doSerialize(z);
    }

   protected:
    virtual void doUpdate() = 0;
    virtual msgpack::object doSerialize(msgpack::zone& z) = 0;

   private:
    mutable std::shared_mutex mtx_;
};
