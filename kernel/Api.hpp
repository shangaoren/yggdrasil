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
#include "Kernel.hpp"
#include "ServiceCall.hpp"
#include "IntVectManager.hpp"
#include "Event.hpp"

#include "yggdrasil/interfaces/ISystem.hpp"


namespace kernel
{
	class Api
	{
	public:
		
		/*Start Kernel with a specified system interruptPriority and a number of subPriority bits*/
		static inline bool startKernel(interfaces::ISystem& system, uint8_t systemInterruptPriority, uint8_t numberOfSubPriorityBits)
		{
			return Scheduler::startKernel(system,systemInterruptPriority,numberOfSubPriorityBits);
		}
		
		/* Start Kernel, assuming settings on system priotity and subPriority are already set, or default will be used */
		static inline bool startKernel(interfaces::ISystem& system)
		{
			return Scheduler::startKernel(system);
		}
		
		/*Set the System Priority, can only be called before installation of interrupts*/
		static inline bool setSystemPriority(uint8_t systemPriority)
		{
			if (!Scheduler::s_interruptInstalled)
			{
				Scheduler::s_systemPriority = systemPriority;
				return true;
			}
			return false;
		}
		
		
		/*Set the Number of SubPriority bits for interrupts, can only be called before installation of interrupts*/
		static inline bool setnumberOfSubPriorityBits(uint8_t numberOfSubs)
		{
			if (!Scheduler::s_interruptInstalled)
			{
				Scheduler::s_subPriorityBits = numberOfSubs;
				return true;
			}
			return false;
		}
		
		
		
		/*wait without using kernel
		 *@Warning : will wait until time counter elapsed*/
		static void wait(uint32_t ms)
		{
			uint64_t endWaitTimeStamp = Scheduler::s_ticks + ms;
			while (Scheduler::s_ticks <= endWaitTimeStamp) ;
		}
		
		/*set a Task into sleep for an amount of ms
		 *@Warning: do not call it if you're not in a Task*/
		static void __attribute__((optimize("O0"))) sleep(uint32_t ms)
		{
			asm volatile(
				"MOV R0,%0\n\t"
				"SVC %[immediate]"::"r" (ms), [immediate] "I"(ServiceCall::SvcNumber::sleepTask));
		}
		
		
		/*get kernel timeStamp*/
		static inline uint64_t getTicks()
		{
			return Scheduler::s_ticks;
		}
		
		/*register an irq before the scheduler has started*/
		static void registerIrq(IRQn_Type irq, IntVectManager::IrqHandler handler)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installSystemInterrupts();
			registerIrqKernel(irq, handler);	
		}
		
		
		/*unregister an irq before the scheduler has started*/
		static void unregisterIrq(IRQn_Type irq)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installSystemInterrupts();
			unRegisterIrqKernel(irq);
		}
		

		/*Setup an Irq Priority*/
		static inline void irqPriority(IRQn_Type irq, uint8_t preEmpt, uint8_t sub)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installSystemInterrupts();
			irqPriorityKernel(irq,preEmpt,sub);
		}

		static inline void irqPriority(IRQn_Type irq, uint8_t priority)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installSystemInterrupts();
			irqGlobalPriorityKernel(irq,priority);
		}

		/*Enable an Irq in NVIC*/
		static inline __attribute__((optimize("O0"))) void enableIrq(IRQn_Type irq)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installSystemInterrupts();
			asm volatile(
					"MOV R0,%0\n\t"
					"SVC %[immediate]"::"r" (irq), [immediate] "I"(ServiceCall::SvcNumber::enableIrq));
		}

		/*Disable an Irq in NVIC*/
		static inline __attribute__((optimize("O0"))) void disableIrq(IRQn_Type irq)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installSystemInterrupts();
			asm volatile(
					"MOV R0,%0\n\t"
					"SVC %[immediate]"::"r" (irq), [immediate] "I"(ServiceCall::SvcNumber::disableIrq));
		}

		/*Clear a pending Irq*/
		static inline __attribute__((optimize("O0"))) void clearIrq(IRQn_Type irq)
		{
			if (Scheduler::s_interruptInstalled)
				Scheduler::installSystemInterrupts();
			asm volatile(
					"MOV R0,%0\n\t"
					"SVC %[immediate]"::"r" (irq), [immediate] "I"(ServiceCall::SvcNumber::clearIrq));
		}
		
		static inline void setupInterrupt(IRQn_Type irq, IntVectManager::IrqHandler handler, uint8_t priority)
		{
			registerIrq(irq, handler);
			irqPriority(irq, priority);
			clearIrq(irq);
			enableIrq(irq);
		}
		
		static inline void setupInterrupt(IRQn_Type irq, IntVectManager::IrqHandler handler, uint8_t preEmpt, uint8_t sub)
		{
			registerIrq(irq, handler);
			irqPriority(irq, preEmpt, sub);
			clearIrq(irq);
			enableIrq(irq);
		}
		
		/*Lock every interrupts below System*/
		static void __attribute__((naked, optimize("O0"))) enterCriticalSection()
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::enterCriticalSection);
			asm volatile("POP {PC}");
		}
		
		/*Unlock Interrupts*/
		static void __attribute__((naked)) exitCriticalSection()
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::exitCriticalSection);
			asm volatile("POP {PC}");
		}

	private:
		/*All Svc function are naked to keep Register with value stored when called*/
		static bool __attribute__((naked)) registerIrqKernel(IRQn_Type irq, IntVectManager::IrqHandler handler)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::registerIrq);
			asm volatile("POP {PC}");
		}
		
		
		static bool __attribute__((naked)) unRegisterIrqKernel(IRQn_Type irq)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::unregisterIrq);
			asm volatile("POP {PC}");
		}
		
		static void __attribute__((naked)) irqGlobalPriorityKernel(IRQn_Type irq, uint8_t priority)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::setGlobalPriority);
			asm volatile("POP {PC}");
		}

		static void __attribute__((naked)) irqPriorityKernel(IRQn_Type irq, uint8_t preEmpt, uint8_t sub)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::setPriority);
			asm volatile("POP {PC}");
		}
	};
	}
