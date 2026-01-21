#pragma once

#include <cstdint>

namespace core {
    class DummySystick {
    public:
        static void init(uint32_t coreFrequency, uint32_t tickFrequency) {
        }
        static void start() {
        }
        static void stop() {
        }
    };
}
