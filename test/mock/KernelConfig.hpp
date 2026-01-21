#pragma once
#include "cstdint"
#include "core/DummyCore.hpp"
#include "core/DummyVector.hpp"

namespace kernel {
    using Core = ::core::DummyCore;
    using Vector = ::core::DummyVector<61+16>;
    class Config {
    public:
        static constexpr uint8_t kernelPriority = 2;
        static constexpr uint32_t systemTimerFrequency = 1000;
    };
}