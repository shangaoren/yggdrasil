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
#include "Scheduler.hpp"


namespace kernel
{
	
	// A task want to get a mutex wait for it if already lock by someone else or get it if free
	bool Mutex::kernelLockMutex(Mutex* mutex, uint32_t timeout)
	{
		uint32_t result = 0;
		Scheduler::enterKernelCriticalSection();
		Y_ASSERT(Scheduler::s_activeTask != nullptr);
		if (mutex->m_locked == false)
		{
			mutex->m_locked = true;
			Scheduler::exitKernelCriticalSection();
			return true;
		}
		else
		{
			mutex->m_waiting.insert(Scheduler::s_activeTask, Task::priorityCompare);
			Scheduler::s_activeTask->m_state = kernel::Task::state::waitingMutex;
			if (Scheduler::s_previousTask == nullptr)
				Scheduler::s_previousTask = Scheduler::s_activeTask;
			Y_ASSERT(Scheduler::s_ready.count() != 0);
			Scheduler::s_activeTask = Scheduler::s_ready.getFirst();
			Scheduler::exitKernelCriticalSection();
			Scheduler::setPendSv(kernel::Scheduler::changeTaskTrigger::waitForMutex);
			return false; // used to return from interrupt
		}
	}
	
	bool Mutex::kernelReleaseMutex(Mutex* mutex)
	{
		if (mutex->m_locked == false)
			return false;
		Scheduler::enterKernelCriticalSection();
		Y_ASSERT(mutex->m_locked == true);
		mutex->m_locked = false;
		if (!mutex->m_waiting.isEmpty())
		{
			Task* newReadyTask = mutex->m_waiting.getFirst();
			Y_ASSERT(newReadyTask != nullptr);
			Y_ASSERT(!Scheduler::s_ready.contain(newReadyTask));   //If the event ready task is already in ready list, we have a problem
			Scheduler::s_ready.insert(newReadyTask, Task::priorityCompare);
			Scheduler::exitKernelCriticalSection();
			Scheduler::schedule(kernel::Scheduler::changeTaskTrigger::wakeByMutex);
		}
		else
		{
			mutex->m_locked = false;
			Scheduler::exitKernelCriticalSection();
		}
		return true;
	}
}// End namespace kernel