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

#include "Mutex.hpp"
#include "Hooks.hpp"
#include "Scheduler.hpp"
#include "core/Core.hpp"


namespace kernel
{
	
	int16_t Mutex::lock(uint32_t timeout)
	{
		Y_ASSERT(Scheduler::inThreadMode());
		return supervisorCallLockMutex(this, timeout);
	}
	
	bool Mutex::release()
	{
		return supervisorCallReleaseMutex(this);
	}
	
	bool Mutex::isLocked()
	{
		return m_owner != nullptr;
	}
	
	// A task want to get a mutex wait for it if already lock by someone else or get it if free
	int16_t Mutex::kernelLockMutex(Mutex* mutex, uint32_t duration)
	{
		int16_t result = 0;
		Y_ASSERT(Scheduler::s_activeTask != nullptr);
		if (mutex->m_owner == nullptr)
		{
			Hooks::onMutexLock(mutex, Scheduler::s_activeTask);
			mutex->m_owner = Scheduler::s_activeTask;
			return 1;
		}
		else
		{
			Y_ASSERT(Scheduler::s_activeTask != nullptr);
			mutex->m_waiting.insert(Scheduler::s_activeTask, Task::priorityCompare);
			if (duration > 0)
			{
				Scheduler::s_activeTask->m_wakeUpTimeStamp = Scheduler::s_ticks + duration;
				Scheduler::s_waiting.insert(Scheduler::s_activeTask, Task::sleepCompare);
			}
			Scheduler::s_activeTask->m_waitingFor = mutex;
			Scheduler::s_activeTask->m_state = kernel::Task::state::waitingMutex;
			Scheduler::s_taskToStack = Scheduler::s_activeTask;
			Scheduler::s_activeTask = Scheduler::s_ready.getFirst();
			Hooks::onMutexWait(mutex, Scheduler::s_taskToStack, duration);
			Scheduler::setPendSv(kernel::Scheduler::changeTaskTrigger::waitForMutex);
			return 1; // used to return from interrupt
		}
	}

	void Mutex::stopWait(Task *task)
	{
		if (task->m_wakeUpTimeStamp != 0)
		{
			Scheduler::s_waiting.remove(task);
			task->m_wakeUpTimeStamp = 0;
		}
	}

	bool Mutex::kernelReleaseMutex(Mutex* mutex)
	{
		Y_ASSERT(mutex != nullptr);
		Hooks::onMutexRelease(mutex);
		if (mutex->m_owner == nullptr) // this should not append
			return false;
		if (!mutex->m_waiting.isEmpty())
		{
			Task* newReadyTask = mutex->m_waiting.getFirst();
			Y_ASSERT(newReadyTask != nullptr);
			Y_ASSERT(!Scheduler::s_ready.contain(newReadyTask)); //If the event ready task is already in ready list, we have a problem
			newReadyTask->m_waitingFor = nullptr;
			mutex->stopWait(newReadyTask);
			Scheduler::s_ready.insert(newReadyTask, Task::priorityCompare);
			newReadyTask->m_state = kernel::Task::state::ready;
			mutex->m_owner = newReadyTask;
			Hooks::onTaskReady(newReadyTask);
			Hooks::onMutexLock(mutex, newReadyTask);
			Scheduler::schedule(kernel::Scheduler::changeTaskTrigger::wakeByMutex);
		}
		else
		{
			mutex->m_owner = nullptr;
		}
		return true;
	}

	void Mutex::onTimeout(Task* task)
	{
		Hooks::onMutexTimeout(this, task);
		Y_ASSERT(m_waiting.contain(task));
		m_waiting.remove(task);
		Y_ASSERT(!m_waiting.contain(task));
		task->m_waitingFor = nullptr; //the task is no more waiting for mutex
		task->setReturnValue(static_cast<int16_t>(-1)); // timeout code
		task->m_wakeUpTimeStamp = 0;
		Scheduler::s_ready.insert(task, Task::priorityCompare);
		task->m_state = kernel::Task::state::ready;
		Hooks::onMutexTimeout(this, task);
		Hooks::onTaskReady(task);
		Scheduler::schedule(kernel::Scheduler::changeTaskTrigger::mutexTimeout);	
	}
	
	Mutex::SupervisorCallLockMutex Mutex::supervisorCallLockMutex  = core::Core::supervisorCall < ServiceCall::SvcNumber::mutexLock, int16_t, Mutex*, uint32_t>;
	Mutex::SupervisorCallReleaseMutex Mutex::supervisorCallReleaseMutex = core::Core::supervisorCall < ServiceCall::SvcNumber::mutexRelease, bool, Mutex*>;

}// End namespace kernel