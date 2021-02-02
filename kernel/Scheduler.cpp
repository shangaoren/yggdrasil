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
#include "core/Core.hpp"
#include "Hooks.hpp"

namespace kernel
{
	bool Scheduler::s_schedulerStarted = false;
	bool Scheduler::s_interruptInstalled = false;
	volatile uint64_t Scheduler::s_ticks = 0;
	volatile Scheduler::changeTaskTrigger Scheduler::s_trigger = Scheduler::changeTaskTrigger::none;		
	Task* volatile Scheduler::s_activeTask = nullptr;
	Task* volatile Scheduler::s_taskToStack = nullptr;

	volatile bool Scheduler::scheduled = false;
	volatile uint8_t Scheduler::s_lockLevel = 0;
	volatile bool Scheduler::s_isKernelLocked = false;
	uint32_t Scheduler::s_sysTickFreq = 1000;
	uint8_t Scheduler::s_systemPriority = 0;

	StartedList Scheduler::s_started;
	ReadyList Scheduler::s_ready;
	SleepingList Scheduler::s_sleeping;
	WaitableList Scheduler::s_waiting;

	/*-------------------------------------------------------------------------------------------*/
	/*                                                                                           */
	/*                                     PUBLIC FONCTION                                       */
	/*                                                                                           */
	/*-------------------------------------------------------------------------------------------*/

	void Scheduler::setupKernel(uint8_t systemPriority)
	{
		s_systemPriority = systemPriority;
		Y_ASSERT(s_systemPriority != 0); // Priority should be > 0
	}

	bool Scheduler::startKernel()
	{
		if (s_schedulerStarted) //Scheduler already started
			return false;
		if (s_ready.isEmpty()) //No Task in task list, cannot start Scheduler
			return false;

		core::Core::idleTask.start(); // Add idle task
		if (!installKernelInterrupt())
			return false;
		//Init Systick
		core::Core::systemTimer.initSystemTimer(core::Core::coreClocks.getSystemCoreFrequency(), s_sysTickFreq);
		core::Core::systemTimer.startSystemTimer();
		Hooks::onKernelStart();
		core::Core::supervisorCall < ServiceCall::SvcNumber::startFirstTask, void>();		 
		return false; //shouldn't end here
	}

	bool Scheduler::installKernelInterrupt()
	{
		if (s_interruptInstalled)
			return true;
		// Install vector manager if not already done
		if (!core::Core::vectorManager.isInstalled())
		{
			if (!core::Core::vectorManager.installVectorManager())
				return false;
		}
		//Setup Systick
		core::Core::vectorManager.irqPriority(core::Core::systemTimer.getIrq(), s_systemPriority);
		core::Core::vectorManager.registerHandler(core::Core::systemTimer.getIrq(), core::Core::systemTimerHandler);

		//Setup Supervisor Call interrupt
		core::Core::vectorManager.irqPriority(core::Core::supervisorIrqNumber, s_systemPriority);
		core::Core::vectorManager.registerHandler(core::Core::supervisorIrqNumber, core::Core::supervisorCallHandler);

		//Setup pendSV interrupt (used for task change)
		core::Core::vectorManager.irqPriority(core::Core::taskSwitchIrqNumber, 0xFF); //Minimum priority for task change
		core::Core::vectorManager.registerHandler(core::Core::taskSwitchIrqNumber, core::Core::contextSwitchHandler);
		s_interruptInstalled = true;
		return true;
	}

	bool __attribute__((aligned(4), optimize("O0"))) Scheduler::startFirstTask()
	{
		//start a task, reset main stack pointer
		s_activeTask = s_ready.getFirst();
		Hooks::onTaskStartExec(s_activeTask);
		s_schedulerStarted = true;
		core::Core::restoreTask(Scheduler::s_activeTask->m_stackPointer);
		return true; //should never return here
	}

	/*-------------------------------------------------------------------------------------------*/
	/*                                                                                           */
	/*                                    PRIVATE FONCTIONS                                      */
	/*                                                                                           */
	/*-------------------------------------------------------------------------------------------*/
	
	
	inline bool Scheduler::irqRegister(Irq irq, core::interfaces::IVectorManager::IrqHandler handler, const char* name)
	{
		core::Core::vectorManager.registerHandler(irq, handler, name);
		return true;
	}
	
	inline bool Scheduler::irqUnregister(Irq irq)
	{
		core::Core::vectorManager.unregisterHandler(irq);
		return true;
	}
	
	inline void Scheduler::enterKernelCriticalSection()
	{
		Y_ASSERT(s_isKernelLocked == false);
		if (s_isKernelLocked == false)
		{
			s_lockLevel = core::Core::vectorManager.lockInterruptsHigherThan(s_systemPriority+1);
			s_isKernelLocked = true;
		}
	}

	inline void Scheduler::exitKernelCriticalSection()
	{
		Y_ASSERT(s_isKernelLocked == true);
		if (s_isKernelLocked == true)
		{
			core::Core::vectorManager.unlockInterruptsHigherThan(s_lockLevel);
			s_isKernelLocked = false;
		}
	}

	bool Scheduler::startTask(Task &task)
	{
		if (task.m_started)
			return false;
		s_started.insert(&task, Task::priorityCompare);
		s_ready.insert(&task, Task::priorityCompare);
		task.m_started = true;
		task.m_state = Task::state::ready;
		Hooks::onTaskStart(&task);
		if (s_schedulerStarted)
			schedule(kernel::Scheduler::changeTaskTrigger::taskStarted);
		return true;
	}

	bool Scheduler::schedule(changeTaskTrigger trigger)
	{
		scheduled = true;
		Y_ASSERT(s_ready.count() != 0); //assertion to check there is ready tasks
		if (s_activeTask != nullptr)
		{
			if (s_ready.peekFirst()->m_taskPriority > s_activeTask->m_taskPriority) //a task with higher priority is waiting, trigger context switching
			{
				//Store currently running task
				Y_ASSERT(!s_ready.contain(s_activeTask)); //currently running task not already in ready list
				Y_ASSERT(!s_sleeping.contain(s_activeTask));
				s_ready.insert(s_activeTask, Task::priorityCompare);
				Hooks::onTaskStopExec(s_activeTask);
				Hooks::onTaskReady(s_activeTask);
				if (s_taskToStack == nullptr)
					s_taskToStack = s_activeTask;
				s_activeTask = nullptr;
				//If there is no task to be saved already, put active task in s_previousTask to save context
				s_taskToStack->m_state = kernel::Task::state::ready;
				//Take the first available task
				s_activeTask = s_ready.getFirst();
				Y_ASSERT(s_activeTask != nullptr);
				setPendSv(trigger);
				return true;
			}
			else
			{
				return false;
			}
		}
		Y_ASSERT(false);
		return false;
	}

	bool Scheduler::stopTask(Task &task)
	{
		s_ready.remove(&task);
		s_sleeping.remove(&task);
		s_started.remove(&task);
		Hooks::onTaskClose(&task);
		if (s_activeTask == &task)
		{
			s_activeTask = s_ready.getFirst();
			core::Core::restoreTask(s_activeTask->m_stackPointer);
		}
		return true;
	}

	bool __attribute__((optimize("O0"))) Scheduler::sleep(uint32_t ms)
	{
		// should be triggered directly from task
		Y_ASSERT(s_activeTask != nullptr);
		Y_ASSERT(s_taskToStack == nullptr);
		s_taskToStack = s_activeTask;
		s_activeTask = nullptr; // no more active task
		s_taskToStack->m_wakeUpTimeStamp = s_ticks + ms;
		//Put active Task to sleep
		Y_ASSERT(!s_sleeping.contain(s_taskToStack)); // if active task already in sleeping list we have a problem
		s_sleeping.insert(s_taskToStack, Task::sleepCompare);
		s_taskToStack->m_state = Task::state::sleeping;
		Hooks::onTaskSleep(s_taskToStack, ms);
		s_activeTask = s_ready.getFirst();
		Y_ASSERT(s_activeTask != nullptr);							 // ready list should at least contain idle task
		setPendSv(kernel::Scheduler::changeTaskTrigger::enterSleep); //active task is sleeping, trigger context switch
		return true;
	}
	
	void Scheduler::setPendSv(changeTaskTrigger trigger)
	{
		s_trigger = trigger;
		core::Core::contextSwitchTrigger();
	}

	bool Scheduler::inThreadMode()
	{
		return core::Core::getCurrentInterruptNumber() == 0;
	}

	volatile uint32_t *__attribute__((optimize("O0"))) Scheduler::taskSwitch(uint32_t *stackPosition)
	{
		if (s_trigger != kernel::Scheduler::changeTaskTrigger::none) // avoid Spurious interrupt
		{
			Y_ASSERT(s_activeTask != nullptr);
			Y_ASSERT(stackPosition > s_taskToStack->m_stackOrigin);
			Y_ASSERT(stackPosition < &s_taskToStack->m_stackOrigin[s_taskToStack->m_stackSize]);
			Y_ASSERT(!s_taskToStack->isStackCorrupted());
			s_taskToStack->setStackPointer(stackPosition);
			Y_ASSERT(s_taskToStack->m_stackUsage < (s_taskToStack->m_stackSize - 48)); //16 + 32 float

			s_taskToStack = nullptr;
			s_activeTask->m_state = kernel::Task::state::active;
			Hooks::onTaskStartExec(s_activeTask);
			s_trigger = kernel::Scheduler::changeTaskTrigger::none;
		}
		else
		{
			//Y_ASSERT(false); //trick to get breakpoint only when debug
			s_activeTask->m_stackPointer = stackPosition;
		}
		scheduled = false;
		return s_activeTask->m_stackPointer;
	}

	void Scheduler::systemTimerTick()
	{
		bool needSchedule = false;
		s_ticks++;
		while (!s_sleeping.isEmpty() && (s_sleeping.peekFirst()->m_wakeUpTimeStamp) <= s_ticks) //one task or more is waiting, let's see if waiting is over
		{
			needSchedule = true;
			Task *readyTask = s_sleeping.getFirst();
			Y_ASSERT(readyTask != nullptr);
			readyTask->m_wakeUpTimeStamp = 0;
			readyTask->m_state = kernel::Task::state::ready;
			Y_ASSERT(!s_ready.contain(readyTask)); //new Ready task should not being already in ready list
			s_ready.insert(readyTask, Task::priorityCompare);
			Hooks::onTaskReady(readyTask);
		}
		while (!s_waiting.isEmpty() && (s_waiting.peekFirst()->m_wakeUpTimeStamp <= s_ticks))
		{
			Task *timeouted = s_waiting.getFirst();
			Y_ASSERT(timeouted->m_waitingFor != nullptr);
			timeouted->m_waitingFor->onTimeout(timeouted);
		}
		if (needSchedule)
			schedule(kernel::Scheduler::changeTaskTrigger::exitSleep);
	}

	void Scheduler::supervisorCall(ServiceCall::SvcNumber t_service, uint32_t *t_args)
	{
		uint32_t param0 = t_args[0], param1 = t_args[1], param2 = t_args[2];
		switch (t_service)
		{
		case ServiceCall::SvcNumber::startFirstTask:
			t_args[0] = startFirstTask(); //write result to stacked R0
			break;

		case ServiceCall::SvcNumber::registerIrq:
			t_args[0] = irqRegister(static_cast<Irq>(param0), reinterpret_cast<core::interfaces::IVectorManager::IrqHandler>(param1), reinterpret_cast<const char *>(param2));
			break;

		case ServiceCall::SvcNumber::unregisterIrq:
			t_args[0] = irqUnregister(static_cast<Irq>(param0)); //execute function and write result to stacked R0
			break;

		case ServiceCall::SvcNumber::startTask:
			t_args[0] = startTask(*(reinterpret_cast<Task *>(param0)));
			break;

		case ServiceCall::SvcNumber::stopTask:
			t_args[0] = stopTask(*(reinterpret_cast<Task *>(param0)));
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

		case ServiceCall::SvcNumber::enableIrq:
			core::Core::vectorManager.enableIrq(static_cast<Irq>(param0));
			break;

		case ServiceCall::SvcNumber::disableIrq:
			core::Core::vectorManager.disableIrq(static_cast<Irq>(param0));
			break;

		case ServiceCall::SvcNumber::clearIrq:
			core::Core::vectorManager.clearIrq(static_cast<Irq>(param0));
			break;

		case ServiceCall::SvcNumber::setGlobalPriority:
			core::Core::vectorManager.irqPriority(static_cast<Irq>(param0), static_cast<uint8_t>(param1));
			break;

		case ServiceCall::SvcNumber::setPriority:
			core::Core::vectorManager.irqPriority(static_cast<Irq>(param0), static_cast<uint8_t>(param1), static_cast<uint8_t>(param2));
			break;
		case ServiceCall::SvcNumber::enterCriticalSection:
			enterKernelCriticalSection();
			break;

		case ServiceCall::SvcNumber::exitCriticalSection:
			exitKernelCriticalSection(); //set basepri to 0 in order to enable all interrupts
			break;

		case kernel::ServiceCall::SvcNumber::mutexLock:
			t_args[0] = Mutex::kernelLockMutex(reinterpret_cast<Mutex *>(param0), param1);
			break;
		case kernel::ServiceCall::SvcNumber::mutexRelease:
			t_args[0] = Mutex::kernelReleaseMutex(reinterpret_cast<Mutex *>(param0));
			break;

		default: //unknown Service call number
			__BKPT(0);
			break;
		}
	}

	uint64_t Scheduler::getTicks()
	{
		return s_ticks;
	}
}	//End namespace kernel
