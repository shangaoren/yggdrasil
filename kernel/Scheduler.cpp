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
    bool Scheduler::s_schedulerStarted = false;
    volatile uint64_t Scheduler::s_ticks = 0;
    TaskController *volatile Scheduler::s_activeTask = nullptr;
    TaskController *volatile Scheduler::s_taskToStack = nullptr;

    volatile bool Scheduler::scheduled = false;
    volatile uint8_t Scheduler::s_lockLevel = 0;
    volatile bool Scheduler::s_isKernelLocked = false;

    StartedList Scheduler::s_started;
    ReadyList Scheduler::s_ready;
    SleepingList Scheduler::s_sleeping;
    WaitableList Scheduler::s_waiting;

    /*-------------------------------------------------------------------------------------------*/
    /*                                                                                           */
    /*                                    PRIVATE FONCTIONS                                      */
    /*                                                                                           */
    /*-------------------------------------------------------------------------------------------*/

    bool Scheduler::startTask(TaskController *task) {
        if (task->m_state != TaskController::State::notStarted)
            return false;
        s_started.insert(task, TaskController::priorityCompare);
        s_ready.insert(task, TaskController::priorityCompare);
        task->m_state = TaskController::State::ready;
        Hooks::onTaskStart(task);
        if (s_schedulerStarted)
            maybeSwitchTask();
        return true;
    }

    bool Scheduler::maybeSwitchTask() {
        scheduled = true;
        y_assert(s_ready.count() != 0); //assertion to check there is ready tasks
        if (s_activeTask == nullptr) {
            y_assert(false);
            return false;
        }
        if (s_ready.peekFirst()->m_priority > s_activeTask->m_priority)
        //a task with higher priority is waiting, trigger context switching
        {
            //Store currently running task
            s_ready.insert(s_activeTask, TaskController::priorityCompare);
            s_activeTask->m_state = TaskController::State::ready;
            Hooks::onTaskStopExec(s_activeTask);
            Hooks::onTaskReady(s_activeTask);
            triggerSwitch();
            return true;
        }
        return false;
    }

    const uint32_t volatile *Scheduler::getStackPointer(const TaskController *task) {
        return task->m_stackPointer;
    }


    bool Scheduler::stopTask(TaskController *task) {
        s_ready.remove(task);
        s_sleeping.remove(task);
        s_started.remove(task);
        if (task->m_waitingFor) {
            task->m_waitingFor->abortWait(task);
        }
        task->m_state = TaskController::State::notStarted;
        Hooks::onTaskClose(task);
        if (s_activeTask == task) {
            s_activeTask = s_ready.getFirst();
            y_assert(s_activeTask != nullptr);
            Core::restoreTask(s_activeTask->m_stackPointer);
        }
        return true;
    }

    bool __attribute__((optimize("O0"))) Scheduler::sleep(const uint32_t ms) {
        // should be triggered directly from task
        y_assert(s_activeTask != nullptr);
        y_assert(s_taskToStack == nullptr);
        s_activeTask->m_wakeUpTimeStamp = static_cast<uint32_t>(s_ticks) + ms;
        //Put active Task to sleep
        y_assert(!s_sleeping.contain(s_taskToStack)); // if active task already in sleeping list we have a problem
        s_sleeping.insert(s_activeTask, TaskController::sleepCompare);
        s_activeTask->m_state = TaskController::State::sleeping;
        Hooks::onTaskSleep(s_taskToStack, ms);
        triggerSwitch(); //active task is sleeping, trigger context switch
        return true;
    }

    void Scheduler::triggerSwitch() {
        Core::contextSwitchTrigger();
    }

    bool Scheduler::inThreadMode() {
        return Core::getCurrentInterruptNumber() == 0;
    }

    volatile uint32_t *__attribute__((optimize("O0"))) Scheduler::taskSwitch(uint32_t *stackPosition) {
            y_assert(s_activeTask != nullptr);
            //TODO add check to verify that task is running inside stack boundaries
            //y_assert(!s_taskToStack->isStackCorrupted());
            s_activeTask->setStackPointer(stackPosition);
            s_activeTask = s_ready.getFirst();
            y_assert(s_activeTask != nullptr);
            s_activeTask->m_state = kernel::TaskController::State::active;
            Hooks::onTaskStartExec(s_activeTask);
        return s_activeTask->m_stackPointer;
    }

    void Scheduler::systemTimerTick() {
        bool maybeNeedsTaskSwitch = false;
        s_ticks = s_ticks + 1;
        while (!s_sleeping.isEmpty() && (s_sleeping.peekFirst()->m_wakeUpTimeStamp) <= s_ticks)
        //one task or more is waiting, let's see if waiting is over
        {
            maybeNeedsTaskSwitch = true;
            TaskController *readyTask = s_sleeping.getFirst();
            y_assert(readyTask != nullptr);
            readyTask->m_wakeUpTimeStamp = 0;
            readyTask->m_state = kernel::TaskController::State::ready;
            y_assert(!s_ready.contain(readyTask)); //new Ready task should not being already in ready list
            s_ready.insert(readyTask, TaskController::priorityCompare);
            Hooks::onTaskReady(readyTask);
        }
        while (!s_waiting.isEmpty() && (s_waiting.peekFirst()->m_wakeUpTimeStamp <= s_ticks)) {
            TaskController *timeouted = s_waiting.getFirst();
            y_assert(timeouted->m_waitingFor != nullptr);
            timeouted->m_waitingFor->onTimeout(timeouted);
        }
        if (maybeNeedsTaskSwitch)
            maybeSwitchTask();
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
                t_args[0] = stopTask(reinterpret_cast<TaskController *>(param0));
                break;
            case ServiceCall::SvcNumber::sleepTask:
                t_args[0] = sleep(param0);
                break;
            case ServiceCall::SvcNumber::signalEvent:
                t_args[0] = Event::kernelSignalEvent(reinterpret_cast<Event *>(param0));
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
                t_args[0] = Mutex::kernelReleaseMutex(reinterpret_cast<Mutex *>(param0));
                break;

            default: //unknown Service call number
                Core::breakpoint();
                break;
        }
    }

    bool __attribute__((aligned(4), optimize("O0"))) Scheduler::startFirstTask() {
        //start a task, reset main stack pointer
        s_activeTask = s_ready.getFirst();
        y_assert(s_activeTask != nullptr);
        Hooks::onTaskStartExec(s_activeTask);
        s_schedulerStarted = true;
        Core::restoreTask(getStackPointer(s_activeTask));
        return true; //should never return here
    }

    void Scheduler::enterKernelCriticalSection() {
          y_assert(s_isKernelLocked == false);
          if (!s_isKernelLocked) {
              //TODO
              //s_lockLevel = Vector::lockInterruptsHigherThan(s_systemPriority + 1);
              s_isKernelLocked = true;
          }
      }

    void Scheduler::exitKernelCriticalSection() {
          y_assert(s_isKernelLocked == true);
          if (s_isKernelLocked) {
              Vector::unlockInterruptsHigherThan(s_lockLevel);
              s_isKernelLocked = false;
          }
      }

    uint64_t Scheduler::getTicks() {
        return s_ticks;
    }

    bool Scheduler::isStarted() {
        return s_schedulerStarted;
    }

} //End namespace kernel
