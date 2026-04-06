# Yggdrasil RTOS — AI Reference Document

> Reference document for AI agents working on projects that use Yggdrasil. Last updated: 2026-04-06

---

## 1. Overview

Yggdrasil is a C++ preemptive RTOS targeting ARM Cortex-M3 and Cortex-M4F microcontrollers.

- No dynamic memory allocation (intrusive lists, statically allocated stacks)
- All kernel operations go through SVC (Supervisor Call)
- Preemptive priority-based scheduler with 1 kHz system tick

---

## 2. Key Concepts

| Concept | File(s) | Description |
|---|---|---|
| **Scheduler** | `kernel/Scheduler.hpp/.cpp` | Preemptive priority-based scheduler. 1 kHz system tick. |
| **Task** | `kernel/Task.hpp/.cpp` | Task with statically allocated stack. Template `Task<StackSize>`. `TaskController` holds logic. |
| **Event** | `kernel/Event.hpp/.cpp` | Inter-task or ISR→task synchronization. Single waiter at a time. Supports timeout. |
| **Mutex** | `kernel/Mutex.hpp/.cpp` | Classic mutex with priority-ordered wait queue and timeout. |
| **ObjectMutex** | `kernel/ObjectMutex.hpp` | Template wrapper coupling a Mutex to an object, returns pointer or nullptr. |
| **Waitable** | `kernel/Waitable.hpp` | Abstract interface for waitable objects (Event, Mutex). Methods: `onTimeout()`, `abortWait()`. |
| **ServiceCall (SVC)** | `kernel/ServiceCall.hpp` | Enum of SVC numbers. Every kernel operation goes through SVC to run in handler mode. |
| **Hooks** | `kernel/Hooks.hpp` | Instrumentation points. Currently: coherence checks (breakpoint if ready list empty with idle in ready). SystemView stubs under `#ifdef SYSVIEW`. |
| **CriticalSection** | `kernel/CriticalSection.hpp/.cpp` | BASEPRI-based locking for kernel-level mutual exclusion. |
| **YList** | `framework/YList.hpp` | Intrusive doubly-linked list. Each TaskController inherits from 4 nodes (Started, Ready, Waitable, Event). |
| **DynamicVector** | `core/cortex_m/DynamicVector.hpp` | Dynamic interrupt vector table (relocated to RAM via VTOR). |

---

## 3. SVC Mechanism (Supervisor Call)

Every kernel operation (sleep, event wait/signal, mutex lock/release, start/stop task, IRQ management) issues an `SVC #N` instruction. The SVC handler (`supervisorCallHandler`):
1. Determines MSP or PSP via EXC_RETURN bit 2
2. Reads the SVC number from the opcode (`LDR R0,[R1,#24]` then `LDRB R0,[R0,#-2]`)
3. Calls `Scheduler::supervisorCall(svcNumber, args)` with stacked registers (R0-R3 = args)
4. Result is written into `t_args[0]` (stacked R0)

**Critical point**: The `SupervisorCallHelper` templates generate inline MOV+SVC instructions. Parameters are passed via R0-R2, return via R0.

### SVC Numbers (`ServiceCall::SvcNumber`)

```
startFirstTask, registerIrq, unregisterIrq, setGlobalPriority, setPriority,
clearIrq, enableIrq, disableIrq, startTask, stopTask, sleepTask,
signalEvent, waitEvent, deleteEvent, enterCriticalSection,
exitCriticalSection, mutexLock, mutexRelease
```

---

## 4. Context Switch

- Triggered via **PendSV** (priority 0xFF — lowest possible)
- `switchCurrentTask()`: moves activeTask to taskToStack, pops first from ready into nextTask
- `taskSwitch()` (called in PendSV): saves PSP of old task, restores the new one
- `maybeSwitchTask()`: checks if the first task in ready has higher priority than the current task

---

## 5. Timeout Handling (Events / Mutex)

- `waitFor(task, ticks)`: inserts the task into the `waiting` list sorted by wakeup timestamp
- `systemTimerTick()`: on each tick, dequeues tasks whose timestamp has been reached
  - If `waitingFor == nullptr` → it's a sleep, move to ready
  - If `waitingFor != nullptr` → call `waitable->onTimeout(task)` (Event or Mutex handles cleanup)
- `Event::onTimeout()`: sets `waiter = nullptr`, `setReturnValue(-1)`, `resume(task)`
- `resume()`: removes from waiting if needed, inserts into ready, calls `maybeSwitchTask()`

---

## 6. Task States

```cpp
enum class State : uint32_t {
    sleeping        = 0,
    active          = 1,
    waitingEvent    = 2,
    notStarted      = 3,
    ready           = 4,
    waitingMutex    = 5,
    nextActive      = 6
};
```

---

## 7. Configuration (`YggdrasilConfig.hpp`)

The host project must provide a `KernelConfig.hpp` (found via `__has_include`) defining:

```cpp
namespace kernel {
    using Core = /* platform Core type */;
    using Vector = core::DynamicVector<N>; // N = number of IRQ + 16 system vectors
    class Config {
    public:
        static constexpr uint8_t kernelPriority = 2;         // SVC and SysTick priority
        static constexpr uint32_t systemTimerFrequency = 1000; // 1 tick = 1 ms
    };
}
```

If no `KernelConfig.hpp` is found, defaults are used (CortexM core, 97 vectors, priority 2, 1 kHz).

---

## 8. Public API (`Yggdrasil.hpp`)

| Method | Description |
|---|---|
| `Yggdrasil::init()` | Installs kernel interrupts (SysTick, SVC, PendSV) |
| `Yggdrasil::start()` | Starts idle task, system timer, and launches the scheduler |
| `Yggdrasil::sleep(ms)` | Cooperative sleep via SVC (must be called from a task) |
| `Yggdrasil::wait(ms)` | Busy-wait outside kernel — does NOT yield |
| `Yggdrasil::getTicks()` | Returns current kernel tick count (uint64_t) |
| `Yggdrasil::setupInterrupt(irq, handler, priority, name)` | Register + configure + enable an IRQ |
| `Yggdrasil::enterCriticalSection()` / `exitCriticalSection()` | Lock/unlock interrupts below kernel priority |
| `Event::wait(duration)` | Wait for event with timeout (returns -1 on timeout) |
| `Event::signal()` | Signal an event (wakes waiter or raises flag) |
| `Mutex::lock(timeout)` / `Mutex::release()` | Mutex operations |

---

## 9. Key File Map

```
src/
├── Yggdrasil.hpp                # Public kernel API
├── YggdrasilConfig.hpp          # Config loader (host KernelConfig.hpp or defaults)
├── kernel/
│   ├── Scheduler.hpp/.cpp       # Scheduler
│   ├── Task.hpp/.cpp            # Tasks
│   ├── Event.hpp/.cpp           # Events
│   ├── Mutex.hpp/.cpp           # Mutex
│   ├── ObjectMutex.hpp          # Typed mutex wrapper
│   ├── Waitable.hpp             # Timeout/abort interface
│   ├── ServiceCall.hpp          # SVC numbers
│   ├── Hooks.hpp                # Instrumentation hooks
│   └── CriticalSection.hpp/.cpp # Critical section
├── core/
│   ├── Processor.hpp            # Processor abstraction
│   └── cortex_m/
│       ├── CortexM.hpp/.cpp     # Default Cortex-M implementation
│       ├── DynamicVector.hpp    # Dynamic vector table
│       └── Irq.hpp              # Type-safe IRQ wrapper
└── framework/
    ├── YList.hpp                # Intrusive doubly-linked list
    └── assert.hpp/.cpp          # y_assert
```

---

## 10. Known Issues & Caveats

### `setReturnValue` offset inconsistency (confirmed bug, low-impact)

```cpp
void setReturnValue(uint32_t value) const {
    auto ctrl = *(stackPointer_ + 8);  // ← reads R10, NOT CONTROL!
    ...
}
void setReturnValue(const int16_t value) const {
    uint32_t ctrl = *(stackPointer_ + 1);  // ← reads CONTROL, correct
    ...
}
```
The `uint32_t` overload reads offset 8 (= R10 on stack) instead of offset 1 (= CONTROL) to detect FPU. **Confirmed bug** but currently not triggered in known code paths (timeouts use the `int16_t` overload).

### `ticks = ++ticks` in `systemTimerTick()`

`++ticks` is sufficient. The self-assignment is a harmless artifact.

### `waitFor()` timestamp truncation

`waitFor()` casts `ticks` (uint64_t) to `uint32_t`. After ~49.7 days → overflow → immediate timeout. Real long-term bug.

### `Mutex::kernelLockMutex` uses `triggerSwitch()` without `switchCurrentTask()`

Inconsistent with Event/Sleep which call `switchCurrentTask()`. Low risk if Mutex is not used in the current config.

---

## 11. Constraints for AI Agents

- **Never call kernel functions (sleep, event.wait, mutex.lock) from an ISR context.** They issue SVC instructions.
- Stack sizes are in **32-bit words**, not bytes.
- Task priorities: higher number = higher priority. Idle = 0.
- ARM interrupt priorities: **lower number = higher priority**. kernelPriority=2 is very high.
- `Yggdrasil::wait()` is a busy-wait outside the kernel. `Yggdrasil::sleep()` is a cooperative sleep via SVC.
- Any scheduler modification must preserve the invariant: **the ready list is never empty** (idle task always present).
- If an ISR with priority ≤ kernelPriority calls `signal()`, it will cause a hard fault (SVC from equal/higher priority handler). There is no "signal from ISR without SVC" path currently.
- `Event` is single-waiter: if two tasks wait on the same Event, the second gets `return 0`. Fragile by design.

