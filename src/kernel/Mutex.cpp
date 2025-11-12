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
	int16_t Mutex::kernelLockMutex(Mutex* mutex, const uint32_t duration)
	{
		const auto currentTask = Scheduler::activeTask.load(std::memory_order_relaxed);
		y_assert(currentTask != nullptr);
		if (mutex->m_owner == nullptr)
		{
			Hooks::onMutexLock(mutex, currentTask);
			mutex->m_owner = currentTask;
			return 1;
		}
		mutex->m_waiting.insertWhen(currentTask, TaskController::priorityCompare);
		if (duration > 0) {
			Scheduler::waitFor(currentTask, duration);
		}
		y_assert(currentTask->waitingFor() == nullptr);
		currentTask->waitingFor(mutex);
		currentTask->state(TaskController::State::waitingMutex);
		Hooks::onMutexWait(mutex, currentTask, duration);
		Scheduler::triggerSwitch();
		return 1; // used to return from interrupt
	}

	void Mutex::abortWait(TaskController *task) {
		y_assert(task->waitingFor() == this);
		y_assert(m_waiting.contain(task));
		m_waiting.erase(task);
		task->waitingFor(nullptr);
		Scheduler::stopWait(task);
	}

	void Mutex::kernelReleaseMutex(Mutex* mutex)
	{
		y_assert(mutex != nullptr);
        y_assert(mutex->m_owner != nullptr);
		Hooks::onMutexRelease(mutex);
		if (!mutex->m_waiting.empty())
		{
			TaskController* newReadyTask = mutex->m_waiting.begin().item();
			y_assert(newReadyTask != nullptr);
			y_assert(newReadyTask->waitingFor() == mutex);
			mutex->m_owner = newReadyTask;
			Hooks::onMutexLock(mutex, newReadyTask);
			Scheduler::resume(newReadyTask);
		}
		else
		{
			mutex->m_owner = nullptr;
		}
	}

	void Mutex::onTimeout(TaskController* task)
	{
		Hooks::onMutexTimeout(this, task);
		y_assert(m_waiting.contain(task));
		m_waiting.erase(task);
		task->waitingFor(nullptr); //the task is no more waiting for mutex
		task->setReturnValue(static_cast<int16_t>(-1)); // timeout code
		Hooks::onMutexTimeout(this, task);
		Scheduler::resume(task);
	}

	Mutex::SupervisorCallLockMutex Mutex::supervisorCallLockMutex  = Core::SupervisorCallHelper<ServiceCall::SvcNumber::mutexLock, int16_t(Mutex*, uint32_t)>::call;
	Mutex::SupervisorCallReleaseMutex Mutex::supervisorCallReleaseMutex = Core::SupervisorCallHelper<ServiceCall::SvcNumber::mutexRelease,bool( Mutex*)>::call;

}// End namespace kernel
