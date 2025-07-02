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
#include <cstdint>
#include "../framework/assert.hpp"
#include "Mutex.hpp"
#include "Hooks.hpp"
#include "Scheduler.hpp"
#include "../YggdrasilConfig.hpp"

namespace kernel
{

	int16_t Mutex::lock(const uint32_t timeout)
	{
		y_assert(Scheduler::inThreadMode());
		return supervisorCallLockMutex(this, timeout);
	}

	bool Mutex::release()
	{
		return supervisorCallReleaseMutex(this);
	}

	bool Mutex::isLocked() const
	{
		return m_owner != nullptr;
	}

	// A task want to get a mutex wait for it if already lock by someone else or get it if free
	int16_t Mutex::kernelLockMutex(Mutex* mutex, uint32_t duration)
	{
		y_assert(Scheduler::s_activeTask != nullptr);
		if (mutex->m_owner == nullptr)
		{
			Hooks::onMutexLock(mutex, Scheduler::s_activeTask);
			mutex->m_owner = Scheduler::s_activeTask;
			return 1;
		}
		else
		{
			mutex->m_waiting.insert(Scheduler::s_activeTask, TaskController::priorityCompare);
			if (duration > 0)
			{
				Scheduler::s_activeTask->m_wakeUpTimeStamp = Scheduler::s_ticks + duration;
				Scheduler::s_waiting.insert(Scheduler::s_activeTask, TaskController::sleepCompare);
			}
			Scheduler::s_activeTask->m_waitingFor = mutex;
			Scheduler::s_activeTask->m_state = kernel::TaskController::State::waitingMutex;
			Scheduler::s_taskToStack = Scheduler::s_activeTask;
			Scheduler::s_activeTask = Scheduler::s_ready.getFirst();
			y_assert(Scheduler::s_activeTask != nullptr);
			Hooks::onMutexWait(mutex, Scheduler::s_taskToStack, duration);
			Scheduler::triggerSwitch();
			return 1; // used to return from interrupt
		}
	}

	void Mutex::stopWait(TaskController *task)
	{
		if (task->m_wakeUpTimeStamp != 0)
		{
			Scheduler::s_waiting.remove(task);
			task->m_wakeUpTimeStamp = 0;
		}
	}

	bool Mutex::kernelReleaseMutex(Mutex* mutex)
	{
		y_assert(mutex != nullptr);
        y_assert(mutex->m_owner != nullptr);
		Hooks::onMutexRelease(mutex);
		if (!mutex->m_waiting.isEmpty())
		{
			TaskController* newReadyTask = mutex->m_waiting.getFirst();
			y_assert(newReadyTask != nullptr);
			y_assert(!Scheduler::s_ready.contain(newReadyTask)); //If the event ready task is already in ready list, we have a problem
			newReadyTask->m_waitingFor = nullptr;
			mutex->stopWait(newReadyTask);
			Scheduler::s_ready.insert(newReadyTask, TaskController::priorityCompare);
			newReadyTask->m_state = kernel::TaskController::State::ready;
			mutex->m_owner = newReadyTask;
			Hooks::onTaskReady(newReadyTask);
			Hooks::onMutexLock(mutex, newReadyTask);
			Scheduler::maybeSwitchTask();
		}
		else
		{
			mutex->m_owner = nullptr;
		}
		return true;
	}

	void Mutex::onTimeout(TaskController* task)
	{
		Hooks::onMutexTimeout(this, task);
		y_assert(m_waiting.contain(task));
		m_waiting.remove(task);
		task->m_waitingFor = nullptr; //the task is no more waiting for mutex
		task->setReturnValue(static_cast<int16_t>(-1)); // timeout code
		task->m_wakeUpTimeStamp = 0;
		Scheduler::s_ready.insert(task, TaskController::priorityCompare);
		task->m_state = kernel::TaskController::State::ready;
		Hooks::onMutexTimeout(this, task);
		Hooks::onTaskReady(task);
	}

	Mutex::SupervisorCallLockMutex Mutex::supervisorCallLockMutex  = Core::SupervisorCallHelper<ServiceCall::SvcNumber::mutexLock, int16_t(Mutex*, uint32_t)>::call;
	Mutex::SupervisorCallReleaseMutex Mutex::supervisorCallReleaseMutex = Core::SupervisorCallHelper<ServiceCall::SvcNumber::mutexRelease,bool( Mutex*)>::call;

}// End namespace kernel
