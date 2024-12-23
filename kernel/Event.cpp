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

#include <cassert>

#include "Scheduler.hpp"
#include "../../core/Core.hpp"
#include "Hooks.hpp"

namespace kernel
{

	Event::~Event() {
		serviceCallEventDelete(this);
	}
	//Task rise an event
	bool Event::kernelSignalEvent(Event* event)
	{
		assert(event != nullptr);
		kernel::Hooks::onEventTrigger(event);
		if (event->m_waiter != nullptr)
		{
			TaskController* newReadyTask = event->m_waiter;
			event->m_waiter = nullptr;
			assert(!Scheduler::s_ready.contain(newReadyTask)); //If the event ready task is already in ready list, we have a problem
			newReadyTask->m_waitingFor = nullptr;
			event->stopWait(newReadyTask);
			Scheduler::s_ready.insert(newReadyTask, TaskController::priorityCompare);
			newReadyTask->m_state = kernel::TaskController::State::ready;
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
		if (event->m_isRaised)	//event already raised, return
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
				Scheduler::s_waiting.insert(Scheduler::s_activeTask, TaskController::sleepCompare);
			}
			Scheduler::s_activeTask->m_waitingFor = event;
			Scheduler::s_activeTask->m_state = TaskController::State::waitingEvent;		   //sets active task as waiting
			Scheduler::s_taskToStack = Scheduler::s_activeTask;
			Scheduler::s_activeTask = Scheduler::s_ready.getFirst();
			Y_ASSERT(Scheduler::s_activeTask != nullptr);
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

	void Event::kernelDeleteEvent(Event *event) {
		if (event->m_waiter != nullptr) {
			TaskController* newReadyTask = event->m_waiter;
			event->m_waiter = nullptr;
			assert(!Scheduler::s_ready.contain(newReadyTask)); //If the event ready task is already in ready list, we have a problem
			newReadyTask->m_waitingFor = nullptr;
			event->stopWait(newReadyTask);
			Scheduler::s_ready.insert(newReadyTask, TaskController::priorityCompare);
			newReadyTask->m_state = kernel::TaskController::State::ready;
			newReadyTask->setReturnValue(static_cast<int16_t>(-1));
			kernel::Hooks::onTaskReady(newReadyTask);
			Scheduler::schedule(kernel::Scheduler::changeTaskTrigger::wakeByEvent);
		}
	}


	bool Event::signal()
	{
		serviceCallEventSignal(this);
		return true;
	}

	bool Event::someoneWaiting() const
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

	bool Event::isAlreadyUp() const
	{
		return m_isRaised;
	}

	void Event::stopWait(TaskController* task)
	{
		if (task->m_wakeUpTimeStamp != 0)
		{
			Scheduler::s_waiting.remove(task);
			task->m_wakeUpTimeStamp = 0;
		}

	}
	void Event::onTimeout(TaskController* task)
	{
		Hooks::onEventTimeout(this);
		assert(task == m_waiter);
		assert(task != nullptr);
		m_waiter = nullptr; // no more task waiting the event
		task->m_waitingFor = nullptr; //the task is no more waiting for event
		task->setReturnValue(static_cast<int16_t>(-1));
		task->m_wakeUpTimeStamp = 0;
		Scheduler::s_ready.insert(task, TaskController::priorityCompare);
		task->m_state = kernel::TaskController::State::ready;
		Hooks::onTaskReady(task);
		Scheduler::schedule(kernel::Scheduler::changeTaskTrigger::wakeByEvent);
	}
    void Event::abortWait(kernel::TaskController *task) {
        if(m_waiter == task){
            m_waiter = nullptr;
            stopWait(task);
            task->m_waitingFor = nullptr;
        }
    }

	Event::SupervisorEventWait Event::serviceCallEventWait = core::Core::SupervisorCallHelper<ServiceCall::SvcNumber::waitEvent,int16_t(Event*, uint32_t)>::call;
	Event::SupervisorEventSignal Event::serviceCallEventSignal = core::Core::SupervisorCallHelper<ServiceCall::SvcNumber::signalEvent,bool (Event*)>::call;
	Event::SupervisorEventDelete Event::serviceCallEventDelete = core::Core::SupervisorCallHelper<ServiceCall::SvcNumber::deleteEvent, void(Event*)>::call;
}

