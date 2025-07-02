#pragma once

#if __has_include("KernelConfig.hpp")
#include "KernelConfig.hpp"
#else
#include "core/CortexM7.hpp"
#include "core/DynamicVector.hpp"
#include "core/vendor/st/stm32f303x8.h"

namespace kernel {
    using Core = core::CortexM7;
    using Vector = core::DynamicVector<97>;
class Config {
public:
    static constexpr uint8_t kernelPriority = 2;
    static constexpr uint32_t systemTimerFrequency = 1000;
};
}
#endif