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

namespace kernel
{
	
	
	class Mutex
	{
		friend class Scheduler;
	public:
		
		constexpr Mutex(): m_locked(false), m_waiting()
		{
		}
		
		bool lock(uint32_t timeout = 0)
		{
			while (!serviceCallLockMutex(this, timeout)) ;
			return true;
		}
		
		bool release()
		{
			return serviceCallReleaseMutex(this);
		}
		
		bool isLocked()
		{
			return m_locked;
		}
		
	private:
		bool m_locked;
		EventList m_waiting;
		
		static bool kernelLockMutex(Mutex* mutex, uint32_t timeout);
		static bool kernelReleaseMutex(Mutex* mutex);
		
		static bool __attribute__((naked, optimize("O0"))) serviceCallLockMutex(Mutex* self, uint32_t timeout)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::mutexLock);
			asm volatile(
				"POP {LR}\n\t"
				"BX LR");
		}
		
		static bool __attribute__((naked, optimize("O0"))) serviceCallReleaseMutex(Mutex* self)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::mutexRelease);
			asm volatile(
				"POP {LR}\n\t"
				"BX LR");
		}
	};
	
template<class ObjectType>
	class ObjectMutex
	{
	public:
		constexpr ObjectMutex(ObjectType& object) : m_object(object), m_mutex()
		{
			
		}
		
		// Obtain a pointer to the object protected by mutex, or nullptr if timeout expired
		ObjectType* get(uint32_t timeout = 0)
		{
			if (m_mutex.lock() == true)
				return &m_object;
			else
				return nullptr;
		}
		
		// release the Object by giving the pointer back, the pointer will be nullptr if success
		bool release(ObjectType* &object)
		{
			if (object != &m_object)	// check that the given object is really the object protected by mutex
				return false;
			if (m_mutex.release())
			{
				object = nullptr;
				return true;
			}
		}
	private:

		ObjectType& m_object;
		Mutex m_mutex;
	};
	}
