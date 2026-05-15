#pragma once

#if __has_include("KernelConfig.hpp")
#include "KernelConfig.hpp"
#else
#define USE_DEFAULT_DYNAMIC_VECTOR
#define USE_DEFAULT_CORTEX_M

#include "core/cortex_m/CortexM.hpp"
#include "core/cortex_m/DynamicVector.hpp"

namespace kernel {
    using Core = core::CortexM;
    using Vector = core::DynamicVector<97>;
class Config {
public:
    static constexpr uint8_t kKernelPriority = 2;
    static constexpr uint32_t kSystemTimerFrequency = 1000;
    static constexpr bool kEnableMailbox = false;
};
}
#endif