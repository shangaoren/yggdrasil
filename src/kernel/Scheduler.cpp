/*MIT License

Copyright (c) 2018 Florian GERARD

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Except as contained in this notice, the name of Florian GERARD shall not be used 
in advertising or otherwise to promote the sale, use or other dealings in this 
Software without prior written authorization from Florian GERARD

*/

#include "Scheduler.hpp"
#include "Hooks.hpp"
#include "Event.hpp"
#include "Mutex.hpp"

namespace kernel {
    bool Scheduler::schedulerStarted = false;
    uint64_t Scheduler::ticks = 0;
    std::atomic<TaskController *> Scheduler::activeTask = nullptr;
    std::atomic<TaskController *> Scheduler::nextTask = nullptr;
    std::atomic<TaskController *> Scheduler::taskToStack = nullptr;
    std::atomic<uint8_t> Scheduler::lockLevel = 0;
    std::atomic<bool> Scheduler::isKernelLocked = false;

    StartedList Scheduler::started;
    ReadyList Scheduler::ready;
    WaitableList Scheduler::waiting;

    /*-------------------------------------------------------------------------------------------*/
    /*                                                                                           */
    /*                                    PRIVATE FUNCTIONS                                      */
    /*                                                                                           */
    /*-------------------------------------------------------------------------------------------*/

    void Scheduler::checkTasks() {
        auto it = Scheduler::started.begin();
        while (it.isValid()) {
            if (it.item()->state() == TaskController::State::ready && !Scheduler::ready.contain(it.item())) {
                core::Core::breakpoint();
            }
            if (it.item()->wakeupTimestamp() && !Scheduler::waiting.contain(it.item())) {
                core::Core::breakpoint();
            }
            ++it;
        }
    }

    void Scheduler::switchCurrentTask() {
        if (taskToStack == nullptr) {
            taskToStack.store(activeTask.load(std::memory_order_relaxed), std::memory_order_relaxed);
            activeTask.store(nullptr, std::memory_order_relaxed);
            triggerSwitch();
        }
        if (nextTask != nullptr) {
            ready.insertWhen(nextTask.load(std::memory_order_relaxed), TaskController::priorityCompare);
        }
        nextTask.store(ready.begin().item(), std::memory_order_relaxed);
    }

    bool Scheduler::startTask(TaskController *task) {
        if (task->m_state != TaskController::State::notStarted)
            return false;
        started.insertWhen(task, TaskController::priorityCompare);
        ready.insertWhen(task, TaskController::priorityCompare);
        task->m_state = TaskController::State::ready;
        Hooks::onTaskStart(task);
        if (schedulerStarted)
            maybeSwitchTask();
        return true;
    }

    bool Scheduler::maybeSwitchTask() {
        y_assert(!ready.empty()); //assertion to check there is ready tasks
        const auto active = activeTask.load(std::memory_order_relaxed);
        y_assert(active != nullptr);
        if (!ready.empty() && ready.begin()->m_priority > active->m_priority) {
            if (active->m_state == TaskController::State::active) {
                ready.insertWhen(active, TaskController::priorityCompare);
                active->m_state = TaskController::State::ready;
                switchCurrentTask();
                Hooks::onTaskStopExec(active);
                Hooks::onTaskReady(active);
            }
            return true;
        }
        return false;
    }

    const uint32_t volatile *Scheduler::getStackPointer(const TaskController *task) {
        return task->m_stackPointer;
    }


    void Scheduler::stopTask(TaskController *task) {
        ready.erase(task);
        started.erase(task);
        if (task->waitingFor()) {
            task->waitingFor()->abortWait(task);
        }
        task->state(TaskController::State::notStarted);
        Hooks::onTaskClose(task);
        if (activeTask == task) {
            activeTask.store(ready.get_and_pop_front().item());
            y_assert(activeTask != nullptr);
            Core::restoreTask(activeTask.load(std::memory_order_relaxed)->m_stackPointer);
        }
    }

    void Scheduler::sleep(const uint32_t ms) {
        const auto currentActive = activeTask.load(std::memory_order_relaxed);
        y_assert(currentActive != nullptr);
        currentActive->wakeupTimestamp(static_cast<uint32_t>(ticks) + ms);
        waiting.insertWhen(currentActive, TaskController::sleepCompare);
        currentActive->m_state = TaskController::State::sleeping;
        Hooks::onTaskSleep(currentActive, ms);
        switchCurrentTask();
        checkTasks();
    }

    void Scheduler::waitFor(TaskController *task, uint32_t waitTicks) {
        y_assert(!waiting.contain(task));
        y_assert(waitTicks != 0);
        task->wakeupTimestamp(static_cast<uint32_t>(ticks) + waitTicks);
        waiting.insertWhen(task, TaskController::sleepCompare);
    }

    void Scheduler::resume(TaskController* task) {
        y_assert(task != nullptr);
        y_assert(!Scheduler::ready.contain(task)); //If the event ready task is already in ready list, we have a problem
        if (task->wakeupTimestamp()) {
            stopWait(task);
        }
        task->state(TaskController::State::ready);
        ready.insertWhen(task, TaskController::priorityCompare);
        y_assert(Scheduler::ready.contain(task));
        y_assert(!Scheduler::waiting.contain(task));
        Hooks::onTaskReady(task);
        checkTasks();
        maybeSwitchTask();
    }

    void Scheduler::stopWait(TaskController *task) {
        y_assert((task->wakeupTimestamp() != 0) ^ !Scheduler::waiting.contain(task));
        if (task->wakeupTimestamp() != 0)
        {
            waiting.erase(task);
            task->wakeupTimestamp(0);
        }
        checkTasks();
    }

    void inline Scheduler::triggerSwitch() {
        Core::contextSwitchTrigger();
    }

    bool Scheduler::inThreadMode() {
        return Core::getCurrentInterruptNumber() == 0;
    }

    volatile uint32_t* Scheduler::taskSwitch(uint32_t *stackPosition) {
        const auto currentActive = taskToStack.load(std::memory_order_relaxed);
        const auto nextActive = nextTask.load(std::memory_order_relaxed);
        y_assert(currentActive != nullptr);
        y_assert(nextActive != nullptr);
        currentActive->setStackPointer(stackPosition);
        nextActive->m_state = kernel::TaskController::State::active;
        activeTask.store(nextActive, std::memory_order_relaxed);
        taskToStack.store(nullptr, std::memory_order_relaxed);
        nextTask.store(nullptr, std::memory_order_relaxed);
        Hooks::onTaskStartExec(activeTask);
        checkTasks();
        return nextActive->m_stackPointer;
    }

    void Scheduler::systemTimerTick() {
        bool maybeNeedsTaskSwitch = false;
        ticks = ++ticks;
        while (!waiting.empty() && (waiting.begin()->wakeupTimestamp() <= ticks)) {
            maybeNeedsTaskSwitch = true;
            TaskController *waiter = waiting.get_and_pop_front().item();
            waiter->wakeupTimestamp(0);
            if (waiter->waitingFor() == nullptr) {
                ready.insertWhen(waiter, TaskController::priorityCompare);
                waiter->state(TaskController::State::ready);
                Hooks::onTaskReady(waiter);
            } else {
                waiter->waitingFor()->onTimeout(waiter);
            }
        }
        checkTasks();
        if (maybeNeedsTaskSwitch) {
            maybeSwitchTask();
        }
    }


    void Scheduler::supervisorCall(const ServiceCall::SvcNumber service, uint32_t *t_args) {
        const uint32_t param0 = t_args[0];
        const uint32_t param1 = t_args[1];
        const uint32_t param2 = t_args[2];
        switch (service) {
            case ServiceCall::SvcNumber::startFirstTask:
                t_args[0] = startFirstTask(); //write result to stacked R0
                break;
            case ServiceCall::SvcNumber::registerIrq:
                Vector::registerHandler(static_cast<core::Irq>(param0),reinterpret_cast<Vector::IrqHandler>(param1),reinterpret_cast<const char *>(param2));
                break;
            case ServiceCall::SvcNumber::unregisterIrq:
                Vector::unregisterHandler(static_cast<core::Irq>(param0));
                break;
            case ServiceCall::SvcNumber::startTask:
                t_args[0] = startTask(reinterpret_cast<TaskController *>(param0));
                break;
            case ServiceCall::SvcNumber::stopTask:
                stopTask(reinterpret_cast<TaskController *>(param0));
                break;
            case ServiceCall::SvcNumber::sleepTask:
                sleep(param0);
                break;
            case ServiceCall::SvcNumber::signalEvent:
                Event::kernelSignalEvent(reinterpret_cast<Event *>(param0));
                break;
            case ServiceCall::SvcNumber::waitEvent:
                t_args[0] = Event::kernelWaitEvent(reinterpret_cast<Event *>(param0), param1);
                break;
            case ServiceCall::SvcNumber::deleteEvent:
                Event::kernelDeleteEvent(reinterpret_cast<Event *>(param0));
                break;
            case ServiceCall::SvcNumber::enableIrq:
                Vector::enableIrq(static_cast<core::Irq>(param0));
                break;

            case ServiceCall::SvcNumber::disableIrq:
                Vector::disableIrq(static_cast<core::Irq>(param0));
                break;

            case ServiceCall::SvcNumber::clearIrq:
                Vector::clearIrq(static_cast<core::Irq>(param0));
                break;

            case ServiceCall::SvcNumber::setGlobalPriority:
                Vector::irqPriority(static_cast<core::Irq>(param0), static_cast<uint8_t>(param1));
                break;

            case ServiceCall::SvcNumber::setPriority:
                Vector::irqPriority(static_cast<core::Irq>(param0), static_cast<uint8_t>(param1),
                                                      static_cast<uint8_t>(param2));
                break;
            case ServiceCall::SvcNumber::enterCriticalSection:
                enterKernelCriticalSection();
                break;

            case ServiceCall::SvcNumber::exitCriticalSection:
                exitKernelCriticalSection();
                break;

            case ServiceCall::SvcNumber::mutexLock:
                t_args[0] = Mutex::kernelLockMutex(reinterpret_cast<Mutex *>(param0), param1);
                break;
            case ServiceCall::SvcNumber::mutexRelease:
                Mutex::kernelReleaseMutex(reinterpret_cast<Mutex *>(param0));
                break;

            default: //unknown Service call number
                Core::breakpoint();
                break;
        }
    }

    bool __attribute__((aligned(4))) Scheduler::startFirstTask() {
        const auto firstTask = ready.get_and_pop_front().item();
        y_assert(firstTask != nullptr);
        Hooks::onTaskStartExec(firstTask);
        schedulerStarted = true;
        firstTask->m_state = TaskController::State::active;
        activeTask.store(firstTask, std::memory_order_relaxed);
        Core::restoreTask(getStackPointer(activeTask));
        return true; //should never return here
    }

    void Scheduler::enterKernelCriticalSection() {
          y_assert(isKernelLocked == false);
          if (!isKernelLocked) {
              lockLevel = Vector::lockInterruptsHigherThan(Config::kernelPriority);
              isKernelLocked = true;
          }
      }

    void Scheduler::exitKernelCriticalSection() {
          y_assert(isKernelLocked == true);
          if (isKernelLocked) {
              Vector::unlockInterruptsHigherThan(lockLevel);
              isKernelLocked = false;
          }
      }

    uint64_t Scheduler::getTicks() {
        return ticks;
    }

    bool Scheduler::isStarted() {
        return schedulerStarted;
    }

} //End namespace kernel
