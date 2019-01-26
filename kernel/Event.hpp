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

#include <cstdint>

#include "Task.hpp" 
#include "ServiceCall.hpp"


namespace kernel
{
	
	class Event
	{
		friend class Scheduler; //let Scheduler access private function but no one else
	public:
		
		Event(bool isRaised = false) : m_isRaised(isRaised)
		{
		}
		
		
		~Event()
		{
			
		}
		
		
				
		//wait for an event, used by Scheduler
		//first task to wait is first task to be served
		//@parameter Task waiting for the event
		//return true if the task is waiting,
		//false if event is already rised
		virtual bool wait()
		{
			waitEventKernel(this);
			return true;
		}
		
		
		//Signal that an event occured
		//if a task is already waiting then return it to wake it up
		//else rise event and return nullptr
		virtual bool signal()
		{
			signalEventKernel(this);
			return true;
		}
		
		virtual bool someoneWaiting()
		{
			if (!m_list.isEmpty())
				return true;
			else
				return false;
		}

		virtual bool isAlreadyUp()
		{
			return m_isRaised;
		}

		virtual void reset()
		{
			m_isRaised = false;
		}

		private:
		
		//------------------PRIVATE DATA------------------------
		EventList m_list;
		bool m_isRaised;
		
		
		
		
		//------------------PRIVATE FUNCTION---------------------
		static bool __attribute__((naked)) waitEventKernel(Event* event)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::waitEvent);
			asm volatile(
				"POP {LR}\n\t"
				"BX LR");
		}
		
		static bool __attribute__((naked)) signalEventKernel(Event* event)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::signalEvent);
			asm volatile(
				"POP {LR}\n\t"
				"BX LR");
		}
	
		};
	

}

