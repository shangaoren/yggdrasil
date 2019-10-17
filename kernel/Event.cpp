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

namespace kernel
{
	//Task rise an event
	bool Event::kernelSignalEvent(Event* event)
	{
		if (!event->m_list.isEmpty())
		{
			Scheduler::enterKernelCriticalSection();
			Task* newReadyTask = event->m_list.getFirst();
			Y_ASSERT(newReadyTask != nullptr);
			Y_ASSERT(!Scheduler::s_ready.contain(newReadyTask));  //If the event ready task is already in ready list, we have a problem
			Scheduler::s_ready.insert(newReadyTask, Task::priorityCompare);
			Scheduler::exitKernelCriticalSection();
			Scheduler::schedule(kernel::Scheduler::changeTaskTrigger::wakeByEvent);
		}
		else
			event->m_isRaised = true;
		return true;
	}
		
		
	//A task ask to wait for an event
	bool Event::kernelWaitEvent(Event* event)
	{
		if (event->m_isRaised)	//event already rised, return
			{
				event->m_isRaised = false;
				return true;
			}
		else	//add task at the end of the waiting list
			{
				Scheduler::enterKernelCriticalSection();
				Y_ASSERT(Scheduler::s_activeTask != nullptr);
				Y_ASSERT(event->m_list.count() == 0);  //should no have multiple waiters
				event->m_list.insert(Scheduler::s_activeTask, Task::priorityCompare);  //insert active task into event waiting list
				Scheduler::s_activeTask->m_state = Task::state::waitingEvent; 	//sets active task as waiting
				if(Scheduler::s_previousTask == nullptr)
					Scheduler::s_previousTask = Scheduler::s_activeTask;  //put active task in previous for context switch, if no context switch is pending
				Y_ASSERT(Scheduler::s_ready.count() != 0);  //should have at least idle task
				Scheduler::s_activeTask = Scheduler::s_ready.getFirst(); 	//take te first available task 
				Scheduler::exitKernelCriticalSection();
				Scheduler::setPendSv(kernel::Scheduler::changeTaskTrigger::waitForEvent);
			}
		return true;
	} 
}// End namespace kernel