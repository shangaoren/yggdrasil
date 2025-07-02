#pragma once

#include <cstdint>
#include <type_traits>

#include "../kernel/ServiceCall.hpp"

#include "DynamicVector.hpp"
#include "Systick.hpp"
#include "../kernel/Task.hpp"

namespace core {
class CortexM7 {
public:

    using SystemTimer = core::HALSystick;
    static constexpr auto systemTimerIrq = Irq(SysTick_IRQn);
    static constexpr auto supervisorCallIrq = Irq(SVCall_IRQn);
    static constexpr auto taskSwitchIrq = Irq(PendSV_IRQn);

    static inline auto idleTask = kernel::Task<50>();

    static void breakpoint();

    static void fatalError();

    static uint32_t getCoreFrequency();

    [[noreturn]] static void idleFunc(uint32_t);

    static inline void contextSwitchTrigger() {
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }

    static void restoreTask(const volatile uint32_t*);

    static void contextSwitchHandler();

    static void supervisorCallHandler();

    static void systemTimerHandler();

    // HardFault interrupt function
    static void hardFault();

    // NMI interrupt function
    static void nmi();

    // usage Fault interrupt function
    static void usageFault();

    // bus fault interrupt function
    static void busFault();

    // HardFaultAnalyzer, called by Hard fault interrupt to investigate
    static void HardFaultAnalyzer(uint32_t *stackPointer);

    // get the number of the current interrupt (0 if thread mode)
    static uint32_t getCurrentInterruptNumber();

            template<kernel::ServiceCall::SvcNumber Number, typename Result, typename... Args>
        struct SupervisorCallHelper;


        template<kernel::ServiceCall::SvcNumber Number, typename Result>
        struct SupervisorCallHelper<Number, Result()> {
            static Result call() {
                if constexpr (std::is_void_v<Result>) {
                    asm volatile(
                        "SVC %[immediate]\n\t"
                        :
                        :[immediate] "I"(Number)
                        :);
                } else {
                    Result functionReturn;
                    asm volatile(
                        "SVC %[immediate]\n\t"
                        "MOV %[result], r0"
                        : [result] "=r" (functionReturn)
                        :[immediate] "I"(Number)
                        :);
                    return functionReturn;
                }
            }
        };


        template<kernel::ServiceCall::SvcNumber Number, typename Result, typename Param0>
        struct SupervisorCallHelper<Number, Result(Param0)> {
            static Result call(Param0 p0) {
                if constexpr (std::is_void_v<Result>) {
                    asm volatile(
                        "MOV r0, %[param0]\n\t"
                        "SVC %[immediate]\n\t"
                        :
                        :[immediate] "I"(Number), [param0] "r" (p0)
                        : "r0");
                } else {
                    Result functionReturn;
                    asm volatile(
                        "MOV r0, %[param0]\n\t"
                        "SVC %[immediate]\n\t"
                        "MOV %[result], r0"
                        : [result] "=r" (functionReturn)
                        :[immediate] "I"(Number), [param0] "r" (p0)
                        : "r0");
                    return functionReturn;
                }
            }
        };


        template<kernel::ServiceCall::SvcNumber Number, typename Result, typename Param0, typename Param1>
        struct SupervisorCallHelper<Number, Result(Param0, Param1)> {
            static Result call(Param0 p0, Param1 p1) {
                if constexpr (std::is_void_v<Result>) {
                    asm volatile(
                        "MOV r0, %[param0]\n\t"
                        "MOV r1, %[param1]\n\t"
                        "SVC %[immediate]\n\t"
                        :
                        :[immediate] "I"(Number), [param0] "r" (p0), [param1] "r" (p1)
                        : "r0", "r1");
                } else {
                    Result functionReturn;
                    asm volatile(
                        "MOV r0, %[param0]\n\t"
                        "MOV r1, %[param1]\n\t"
                        "SVC %[immediate]\n\t"
                        "MOV %[result], r0"
                        : [result] "=r" (functionReturn)
                        :[immediate] "I"(Number), [param0] "r" (p0), [param1] "r" (p1)
                        : "r0", "r1");
                    return functionReturn;
                }
            }
        };

        template<kernel::ServiceCall::SvcNumber Number, typename Result, typename Param0, typename Param1, typename
            Param2>
        struct SupervisorCallHelper<Number, Result(Param0, Param1, Param2)> {
            static Result call(Param0 p0, Param1 p1, Param2 p2) {
                if constexpr (std::is_void_v<Result>) {
                    asm volatile(
                        "MOV r0, %[param0]\n\t"
                        "MOV r1, %[param1]\n\t"
                        "MOV r2, %[param2]\n\t"
                        "SVC %[immediate]\n\t"
                        :
                        :[immediate] "I"(Number), [param0] "r" (p0), [param1] "r" (p1), [param2] "r" (p2)
                        : "r0", "r1", "r2");
                } else {
                    Result functionReturn;
                    asm volatile(
                        "MOV r0, %[param0]\n\t"
                        "MOV r1, %[param1]\n\t"
                        "MOV r2, %[param2]\n\t"
                        "SVC %[immediate]\n\t"
                        "MOV %[result], r0"
                        : [result] "=r" (functionReturn)
                        :[immediate] "I"(Number), [param0] "r" (p0), [param1] "r" (p1), [param2] "r" (p2)
                        : "r0", "r1", "r2");
                    return functionReturn;
                }
            }
        };
};
}
