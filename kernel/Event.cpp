/*MIT License

Copyright (c) 2019 Florian GERARD

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

#include "Event.hpp"
#include "Scheduler.hpp"
#include "core/Core.hpp"
#include "Hooks.hpp"

namespace kernel
{
	//Task rise an event
	bool Event::kernelSignalEvent(Event* event)
	{
		Y_ASSERT(event != nullptr);
		kernel::Hooks::onEventTrigger(event);
		if (event->m_waiter != nullptr)
		{
			Task* newReadyTask = event->m_waiter;
			event->m_waiter = nullptr;
			Y_ASSERT(!Scheduler::s_ready.contain(newReadyTask)); //If the event ready task is already in ready list, we have a problem
			newReadyTask->m_waitingFor = nullptr;
			event->stopWait(newReadyTask);
			Y_ASSERT(!Scheduler::s_waiting.contain(newReadyTask)); 
			Scheduler::s_ready.insert(newReadyTask, Task::priorityCompare);
			newReadyTask->m_state = kernel::Task::state::ready;
			kernel::Hooks::onTaskReady(newReadyTask);
			Scheduler::schedule(kernel::Scheduler::changeTaskTrigger::wakeByEvent);
		}
		else
			event->m_isRaised = true;
		return true;
	}
		
		
	//A task ask to wait for an event
	// return 1 if wait success, 0 if unable to wait, -1 if timeout
	int16_t Event::kernelWaitEvent(Event* event, uint32_t duration)
	{
		if (event->m_isRaised)	//event already rised, return
		{
			event->m_isRaised = false;
			return 1;
		}
		else	//add task at the end of the waiting list
		{

			if (event->m_waiter != nullptr)
				return 0;
			//no need to lock as we are in SVC so nothing should interrupt and write this
			Y_ASSERT(Scheduler::s_activeTask != nullptr);
			event->m_waiter = Scheduler::s_activeTask; //insert active task into event waiting list
			if (duration > 0)
			{
				Scheduler::s_activeTask->m_wakeUpTimeStamp = Scheduler::s_ticks + duration;
				Scheduler::s_waiting.insert(Scheduler::s_activeTask, Task::sleepCompare);
			}
			Scheduler::s_activeTask->m_waitingFor = event;
			Scheduler::s_activeTask->m_state = Task::state::waitingEvent;		   //sets active task as waiting
			Scheduler::s_taskToStack = Scheduler::s_activeTask;
			Scheduler::s_activeTask = Scheduler::s_ready.getFirst();
			Hooks::onTaskWaitEvent(Scheduler::s_taskToStack, event);
			Scheduler::setPendSv(kernel::Scheduler::changeTaskTrigger::waitForEvent);
			return 1;
		}		
	}
	
	int16_t Event::wait(uint32_t duration)
	{
		Y_ASSERT(Scheduler::inThreadMode());
		return serviceCallEventWait(this, duration);
	}
	
	bool Event::signal()
	{
		serviceCallEventSignal(this);
		return true;
	}
		
	bool Event::someoneWaiting()
	{
		if (m_waiter != nullptr)
			return true;
		else
			return false;
	}
	
	void Event::reset()
	{
		m_isRaised = false;
	}
	
	bool Event::isAlreadyUp()
	{
		return m_isRaised;
	}

	void Event::stopWait(Task* task)
	{
		if (task->m_wakeUpTimeStamp != 0)
		{
			Scheduler::s_waiting.remove(task);
			task->m_wakeUpTimeStamp = 0;
		}
		
	}
	void Event::onTimeout(Task* task)
	{
		Hooks::onEventTimeout(this);
		Y_ASSERT(task == m_waiter);
		Y_ASSERT(task != nullptr);
		m_waiter = nullptr; // no more task waiting the event
		task->m_waitingFor = nullptr; //the task is no more waiting for event
		task->setReturnValue(static_cast<int16_t>(-1));
		task->m_wakeUpTimeStamp = 0;
		Scheduler::s_ready.insert(task, Task::priorityCompare);
		task->m_state = kernel::Task::state::ready;
		Hooks::onTaskReady(task);
		Scheduler::schedule(kernel::Scheduler::changeTaskTrigger::wakeByEvent);
	}
	
	Event::SupervisorEventWait Event::serviceCallEventWait = core::Core::supervisorCall<ServiceCall::SvcNumber::waitEvent, int16_t, Event*, uint32_t>;
	Event::SupervisorEventSignal Event::serviceCallEventSignal = core::Core::supervisorCall<ServiceCall::SvcNumber::signalEvent, bool, Event*>;
}// End namespace kernel

