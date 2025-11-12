#pragma once

#include <cstdint>

#include "src/core/vendor/st/stm32f303x8.h"


namespace core {
    class HALSystick {
    public:
        static void init(uint32_t coreFrequency, uint32_t tickFrequency) {
            SysTick->LOAD = (SysTick->LOAD & ~SysTick_LOAD_RELOAD_Msk) | (((coreFrequency / tickFrequency) - 1u) << SysTick_LOAD_RELOAD_Pos);
            SysTick->VAL = (SysTick->VAL & ~SysTick_VAL_CURRENT_Msk) | (0 << SysTick_VAL_CURRENT_Pos);
            SysTick->CTRL = (SysTick->CTRL & ~(SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk )) | ((1U << SysTick_CTRL_CLKSOURCE_Pos)| (1U << SysTick_CTRL_TICKINT_Pos));
            NVIC_EnableIRQ(SysTick_IRQn);
        }
        static void start() {
            SysTick->CTRL  |= SysTick_CTRL_ENABLE_Msk;
        }
        static void stop() {
            SysTick->CTRL  &= ~SysTick_CTRL_ENABLE_Msk;
        }
    };
}
