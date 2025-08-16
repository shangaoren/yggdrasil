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
#include "yggdrasil/kernel/Waitable.hpp"


namespace kernel
{
	
	class Event final: public Waitable
	{
		friend class Scheduler; //let Scheduler access private function but no one else
	public:
		constexpr explicit Event(const char*name = nullptr) : m_name(name)
		{
		}

		Event(const Event &) = delete;
		Event(Event &&) = delete;

		Event &operator=(Event &&) = delete;
		Event &operator=(const Event &) = delete;
		virtual ~Event();


		//wait for an event, used by Scheduler
		//first task to wait is first task to be served
		//@parameter Task waiting for the event
		//return true if the task is waiting,
		//false if event is already risen
		int16_t wait(uint32_t duration);


		//Signal that an event occurred
		//if a task is already waiting then return it to wake it up
		//else rise event and return nullptr
		bool signal();

		[[nodiscard]] bool someoneWaiting() const;

		[[nodiscard]] bool isAlreadyUp() const ;

		void reset();
    protected:
		void stopWait(TaskController* task) override;
		void onTimeout(TaskController* task) override;
        void abortWait(TaskController* task) override;

		static bool kernelSignalEvent(Event* event);
		static int16_t kernelWaitEvent(Event* event, uint32_t duration);
		static void kernelDeleteEvent(Event* event);

	  private:

		//------------------PRIVATE DATA------------------------
		TaskController* volatile m_waiter = nullptr;
		bool m_isRaised = false;
		const char *m_name;

		//------------------PRIVATE FUNCTIONS---------------------

		using SupervisorEventWait = int16_t(&)(Event*, uint32_t);
		static SupervisorEventWait& serviceCallEventWait;

		using SupervisorEventSignal = bool(&)(Event*);
		static SupervisorEventSignal& serviceCallEventSignal;

		using SupervisorEventDelete = void(&)(Event*);
		static SupervisorEventDelete& serviceCallEventDelete;
	
		};
	

}

