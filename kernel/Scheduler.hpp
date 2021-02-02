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

#include "Event.hpp"
#include "Mutex.hpp"
#include "ServiceCall.hpp"
#include "Task.hpp"
#include <array>

#include "yggdrasil/framework/Assertion.hpp"
#include "yggdrasil/framework/DualLinkedList.hpp"
#include "yggdrasil/interfaces/IVectorsManager.hpp"
#include "yggdrasil/interfaces/IWaitable.hpp"
#include "yggdrasil/framework/DualLinkedList.hpp"
#include "yggdrasil/framework/Assertion.hpp"


namespace core
{
	class Core;
}
namespace kernel
{
	using namespace core::interfaces;
	class Scheduler
	{
		friend class ServiceCall;
		friend class Api;
		friend class SystemView;
		friend class Task;
		friend class Mutex;
		friend class Event;
		friend class ::core::Core;

	  private:
		enum class changeTaskTrigger : uint32_t
		{
			enterSleep = 0,
			exitSleep = 1,
			waitForEvent = 2,
			wakeByEvent = 3,
			taskStarted = 4,
			taskStopped = 5,
			waitForMutex = 6,
			wakeByMutex = 7,
			eventTimeout = 8,
			mutexTimeout = 9,
			none = 20,
		};

		static void setupKernel(uint8_t systemPriority);

		static bool startKernel();

		static bool installKernelInterrupt();

		static bool startFirstTask();

		/*****************************************************DATA*****************************************************/

		/* Interrupts Variables */
		static uint8_t s_systemPriority;
		static bool s_interruptInstalled;

		/* Tasks Lists */
		static ReadyList s_ready;
		static SleepingList s_sleeping;
		static StartedList s_started;
		static WaitableList s_waiting;

		/* Task Related Variables */
		static volatile changeTaskTrigger s_trigger;
		static Task *volatile s_activeTask;
		static Task *volatile s_taskToStack;
		static volatile bool scheduled;
		static volatile uint8_t s_lockLevel; // store the level of lock before critical section enters
		static volatile bool s_isKernelLocked; // indicates if the kernel is in a critical section mode 
		static uint32_t s_sysTickFreq;

		/* Scheduler misc */
		static bool s_schedulerStarted;
		volatile static uint64_t s_ticks;

		/****************************************************FUNCTIONS*************************************************/

		//register an Irq, only accessed via service call
		static bool irqRegister(Irq irq, core::interfaces::IVectorManager::IrqHandler handler, const char* name);
		
		
		//unregister an Irq, only accessed via service call
		static bool irqUnregister(Irq irq);
		
		
		/*Lock all interrupt lower or equal of system*/
		static void enterKernelCriticalSection();
		
		/*release Interrupt lock*/
		static void exitKernelCriticalSection();

		//start a task
		static bool startTask(Task &task);

		/**
		 * Look at ready task to see if a context switching is needed
		 ***/
		static bool schedule(changeTaskTrigger trigger);
		static void asmPendSv();
		static void asmSvcHandler();
		/*Stop a Task*/
		/*TODO Implement*/
		static bool stopTask(Task &task);
		//function to sleep a task for a number of ms
		static bool sleep(uint32_t ms);
		static volatile uint32_t* taskSwitch(uint32_t *stackPosition);
		//set pendSv, trigger context switch
		static void setPendSv(changeTaskTrigger trigger);
		//static void checkStack();
		static bool inThreadMode();
		//static void stopWait(interfaces::IWaitable *waitable);
		//static void wait(interfaces::IWaitable *waitable);
		//systick handler
		static void systemTimerTick();
		static void svcBootstrap();
		//Svc handler, redirect svc call to the right function
		static void supervisorCall(ServiceCall::SvcNumber t_service, uint32_t *t_args);

	private:
		//Function of Idle Task (NOP when NDEBUG is defined, WFI when release)
		static void idleTaskFunction(uint32_t);
		static uint64_t getTicks();
	};
} // namespace kernel
