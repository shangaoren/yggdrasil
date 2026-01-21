#pragma once
#ifdef USE_DUMMY_CORE
#include <cstdint>
#include <type_traits>
#include "../../../src/kernel/ServiceCall.hpp"
#include "../../../src/kernel/Scheduler.hpp"
#include "../../../src/core/cortex_m/Irq.hpp"
#include "DummySystick.hpp"

namespace core {
class DummyCore {
public:

    using SystemTimer = core::DummySystick;
    static constexpr auto systemTimerIrq = Irq(4);
    static constexpr auto supervisorCallIrq = Irq(5);
    static constexpr auto taskSwitchIrq = Irq(6);

    static inline auto idleTask = kernel::Task<50>();

    static void breakpoint() {
        for (;;);
    }

    static void fatalError() {
        for (;;);
    }

    static uint32_t getCoreFrequency() {
        return 0;
    }

    [[noreturn]] static void idleFunc(uint32_t) {
        for (;;);
    }

    static inline void contextSwitchTrigger() {
    }

    static void restoreTask(const volatile uint32_t*) {

    }

    static void contextSwitchHandler() {

    }

    static void supervisorCallHandler() {

    }

    static void systemTimerHandler() {
        //kernel::Scheduler::systemTimerTick();
    }

    // HardFault interrupt function
    static void hardFault() {

    }

    // NMI interrupt function
    static void nmi() {

    }

    // usage Fault interrupt function
    static void usageFault() {

    }

    // bus fault interrupt function
    static void busFault() {

    }

    // HardFaultAnalyzer, called by Hard fault interrupt to investigate
    static void HardFaultAnalyzer(uint32_t *stackPointer) {

    }

    // get the number of the current interrupt (0 if thread mode)
    static uint32_t getCurrentInterruptNumber() {
        return 1;
    }

            template<kernel::ServiceCall::SvcNumber Number, typename Result, typename... Args>
        struct SupervisorCallHelper;


        template<kernel::ServiceCall::SvcNumber Number, typename Result>
        struct SupervisorCallHelper<Number, Result()> {
            static Result call() {
            }
        };


        template<kernel::ServiceCall::SvcNumber Number, typename Result, typename Param0>
        struct SupervisorCallHelper<Number, Result(Param0)> {
            static Result call(Param0 p0) {
            }
        };


        template<kernel::ServiceCall::SvcNumber Number, typename Result, typename Param0, typename Param1>
        struct SupervisorCallHelper<Number, Result(Param0, Param1)> {
            static Result call(Param0 p0, Param1 p1) {
            }
        };

        template<kernel::ServiceCall::SvcNumber Number, typename Result, typename Param0, typename Param1, typename
            Param2>
        struct SupervisorCallHelper<Number, Result(Param0, Param1, Param2)> {
            static Result call(Param0 p0, Param1 p1, Param2 p2) {
            }
        };
};
}
#endif
