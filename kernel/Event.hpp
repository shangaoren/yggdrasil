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

#pragma once

#include <cstdint>

#include "Task.hpp" 
#include "ServiceCall.hpp"
#include "yggdrasil/interfaces/IWaitable.hpp"



namespace kernel
{
	
	class Event : public interfaces::IWaitable
	{
		friend class Scheduler; //let Scheduler access private function but no one else
	public:
		
		constexpr Event(bool isRaised = false, const char*name = nullptr) :m_waiter(nullptr), m_isRaised(isRaised), m_name(name)
		{
		}
		
		
		~Event()
		{
			
		}
		
		static bool kernelSignalEvent(Event* event);
		static int16_t kernelWaitEvent(Event* event, uint32_t duration);
		
		
				
		//wait for an event, used by Scheduler
		//first task to wait is first task to be served
		//@parameter Task waiting for the event
		//return true if the task is waiting,
		//false if event is already rised
		virtual inline int16_t wait(uint32_t duration = 0)
		{
			return serviceCallEventWait(this, duration);
		}
		
		
		//Signal that an event occured
		//if a task is already waiting then return it to wake it up
		//else rise event and return nullptr
		virtual inline bool signal()
		{
			serviceCallEventSignal(this);
			return true;
		}
		
		virtual bool someoneWaiting()
		{
			if (m_waiter != nullptr)
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

		void stopWait(Task* task) final;
		void onTimeout(Task* task) final;

	  private:
		
		//------------------PRIVATE DATA------------------------
		Task* volatile m_waiter;
		bool m_isRaised;
		const char *m_name;

		//------------------PRIVATE FUNCTIONS---------------------
		static int16_t __attribute__((naked, optimize("O0"))) serviceCallEventWait(Event* event, uint32_t duration)
		{
			asm volatile("PUSH {LR}\n\t"
						"DSB \n\t"
						 "SVC %[immediate]\n\t"
						 "POP {LR}\n\t"
						 "BX LR" ::[immediate] "I"(ServiceCall::SvcNumber::waitEvent));
		}
		
		static bool __attribute__((naked, optimize("O0"))) serviceCallEventSignal(Event* event)
		{
			asm volatile("PUSH {LR}\n\t"
						 "DSB \n\t"
						 "SVC %[immediate]\n\t"
						 "POP {LR}\n\t"
						 "BX LR"::[immediate] "I"(ServiceCall::SvcNumber::signalEvent)	);
		}
		
		//------------------PRIVATE KERNEL FUNCTIONS---------------------
	
		};
	

}

