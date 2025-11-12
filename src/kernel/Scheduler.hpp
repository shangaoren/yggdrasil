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

#pragma once

#include "Task.hpp"
#include "../YggdrasilConfig.hpp"
#include "ServiceCall.hpp"

namespace kernel {
    class Scheduler {
        friend class SystemView;
        friend class TaskController;
        friend class Event;
        friend class Mutex;
        friend class Yggdrasil;
        friend class Hooks;
        friend Core;

    private:

        /*****************************************************DATA*****************************************************/

        /* Tasks Lists */
        static ReadyList ready;
        static StartedList started;
        static WaitableList waiting;

        /* Task Related Variables */
        static std::atomic<TaskController*> taskToStack;
        static std::atomic<TaskController*> activeTask;
        static std::atomic<TaskController*> nextTask;
        static std::atomic<uint8_t> lockLevel; // store the level of lock before critical section enters
        static std::atomic<bool> isKernelLocked; // indicates if the kernel is in a critical section mode

        /* Scheduler misc */
        static bool schedulerStarted;
        static uint64_t ticks;

        /****************************************************FUNCTIONS*************************************************/

        static void checkTasks();

        static void switchCurrentTask();

        /*Lock all interrupt lower or equal of system*/
        static void enterKernelCriticalSection();

        /*release Interrupt lock*/
        static void exitKernelCriticalSection();

        //start a task
        static bool startTask(TaskController *task);

        static const uint32_t volatile *getStackPointer(const TaskController *task);

        /**
         * Look at ready task to see if a context switching is needed
         ***/
        static bool maybeSwitchTask();

        /*Stop a Task*/
        static void stopTask(TaskController *task);

        //function to sleep a task for a number of ms
        static void sleep(uint32_t ms);

        static void waitFor(TaskController *task, uint32_t ticks);

        static void resume(TaskController* task);

        static void stopWait(TaskController *task);

        static volatile uint32_t *taskSwitch(uint32_t *stackPosition);

        static void triggerSwitch();

        static void systemTimerTick();

        static void supervisorCall(ServiceCall::SvcNumber service, uint32_t *t_args);

        static bool __attribute__((aligned(4))) startFirstTask();

    public:
        static bool inThreadMode();

        static uint64_t getTicks();
        static bool isStarted();
    };
} // namespace kernel
