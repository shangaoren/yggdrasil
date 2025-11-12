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
#include "Task.hpp"
#include "yggdrasil/src/kernel/Waitable.hpp"


namespace kernel
{
	class Mutex final: public Waitable
	{
		friend class Scheduler;
	public:
		
		constexpr Mutex()= default;

		/* try to lock resource,
		 * timeout specify a time in ms to wait for resource, 0 for no timeout
		 * return 1 if wait success, 0 if unable to wait, -1 if timeout*/	
		int16_t lock(uint32_t timeout = 0);
		
		bool release();
		
		[[nodiscard]] bool isLocked() const;

		void abortWait(TaskController *task) override;
		
	private:
		EventList m_waiting;
		TaskController *m_owner = nullptr;

		static int16_t kernelLockMutex(Mutex* mutex, uint32_t duration);
		static void kernelReleaseMutex(Mutex* mutex);
		void onTimeout(TaskController* task) override;
		
		// call kernel to lock mutex
		//@return int16_t, 1 if when success, -1 if timeout, 0 if error
		//@params pointer to mutex to lock, optional timeout (0 to disable)
		using SupervisorCallLockMutex = int16_t(&)(Mutex*, uint32_t);
		static SupervisorCallLockMutex& supervisorCallLockMutex;

		// call kernel to unlock mutex
		//@return bool, true if success, false otherwise
		//@params pointer to mutex to unlock
		using SupervisorCallReleaseMutex = bool(&)(Mutex*);
		static SupervisorCallReleaseMutex& supervisorCallReleaseMutex;
	};
	}
