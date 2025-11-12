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

#include "../framework/assert.hpp"
#include "Event.hpp"
#include "Hooks.hpp"
#include "Scheduler.hpp"
#include "../YggdrasilConfig.hpp"


namespace kernel
{

	Event::~Event() {
		serviceCallEventDelete(this);
	}
	//Task rise an event
	void Event::kernelSignalEvent(Event* event)
	{
		y_assert(event != nullptr);
		Hooks::onEventTrigger(event);
		if (event->waiter != nullptr)
		{
			TaskController* newReadyTask = event->waiter;
			y_assert(newReadyTask->waitingFor() == event);
			event->waiter = nullptr;
			newReadyTask->waitingFor(nullptr);
			Scheduler::resume(newReadyTask);
		}
		else
			event->isRaised = true;
	}


	//A task ask to wait for an event
	// return 1 if wait success, 0 if unable to wait, -1 if timeout
	int16_t Event::kernelWaitEvent(Event* event, uint32_t duration)
	{
		y_assert(event != nullptr);
		const auto currentTask = Scheduler::activeTask.load();
		y_assert(currentTask != nullptr);
		y_assert(currentTask->waitingFor() == nullptr);
		if (event->isRaised)	//event already raised, return
		{
			event->isRaised = false;
			return 1;
		}
		//add task at the end of the waiting list
		if (event->waiter != nullptr) {
			return 0;
		}
		event->waiter = currentTask; //insert active task into event waiting list
		if (duration > 0) {
			Scheduler::waitFor(Scheduler::activeTask, duration);
		}
		currentTask->waitingFor(event);
		currentTask->m_state = TaskController::State::waitingEvent; //sets active task as waiting
		Hooks::onTaskWaitEvent(currentTask, event);
		Scheduler::triggerSwitch();
		return 1;
	}

	int16_t Event::wait(const uint32_t duration)
	{
		y_assert(Scheduler::inThreadMode());
		return serviceCallEventWait(this, duration);
	}

	void Event::kernelDeleteEvent(Event *event) {
		y_assert(event != nullptr);
		if (event->waiter) {
			TaskController *newReadyTask = event->waiter;
			y_assert(newReadyTask->waitingFor() == event);
			event->waiter = nullptr;
			newReadyTask->waitingFor(nullptr);
			newReadyTask->setReturnValue(static_cast<int16_t>(-1));
			Scheduler::resume(newReadyTask);
			y_assert(newReadyTask->waitingFor() == nullptr);
		}
	}


	bool Event::signal()
	{
		serviceCallEventSignal(this);
		return true;
	}

	bool Event::someoneWaiting() const
	{
		return waiter != nullptr;
	}

	void Event::reset()
	{
		isRaised = false;
	}

	bool Event::isAlreadyUp() const
	{
		return isRaised;
	}

	void Event::onTimeout(TaskController* task)
	{
		Hooks::onEventTimeout(this);
		y_assert(task != nullptr);
		y_assert(task == waiter);
		y_assert(task->waitingFor() == this);
		waiter = nullptr; // no more task waiting the event
		task->waitingFor(nullptr); //the task is no more waiting for event
		task->setReturnValue(static_cast<int16_t>(-1));
		Scheduler::resume(task);
		y_assert(task->waitingFor() == nullptr);
	}
    void Event::abortWait(TaskController *task) {
		y_assert(waiter == task);
		waiter = nullptr;
		Scheduler::stopWait(task);
		task->waitingFor(nullptr);
    }

	Event::SupervisorEventWait Event::serviceCallEventWait = Core::SupervisorCallHelper<ServiceCall::SvcNumber::waitEvent,int16_t(Event*, uint32_t)>::call;
	Event::SupervisorEventSignal Event::serviceCallEventSignal = Core::SupervisorCallHelper<ServiceCall::SvcNumber::signalEvent,bool (Event*)>::call;
	Event::SupervisorEventDelete Event::serviceCallEventDelete = Core::SupervisorCallHelper<ServiceCall::SvcNumber::deleteEvent, void(Event*)>::call;
}

