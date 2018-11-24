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
		static inline bool startKernel(interface::ISystem& systemClock, uint8_t systemInterruptPriority, uint8_t numberOfSubPriorityBits)
		{
			return Scheduler::startKernel(systemClock,systemInterruptPriority,numberOfSubPriorityBits);
		}
		
		
		/*wait without using kernel*/
		static void wait(uint32_t ms)
		{
			uint64_t endWaitTimeStamp = Scheduler::s_ticks + ms;
			while (Scheduler::s_ticks <= endWaitTimeStamp) ;
		}
		
		static void __attribute__((aligned(4))) sleep(uint32_t ms)
		{
			asm volatile(
				"MOV R0,%0\n\t"
				"SVC %[immediate]"::"r" (ms), [immediate] "I"(ServiceCall::SvcNumber::sleepTask));
		}
		
		
		/*get kernel timeStamp*/
		static uint64_t getTicks()
		{
			return Scheduler::s_ticks;
		}
		
		/*register an irq before the scheduler has started*/
		static void registerIrq(IRQn_Type irq, IntVectManager::IrqHandler handler)
		{
			if (Scheduler::s_schedulerStarted)
				registerIrqKernel(irq, handler);
			else
				Scheduler::preSchedulerIrqRegister(irq, handler);
		}
		
		
		/*unregister an irq before the scheduler has started*/
		static void unregisterIrq(IRQn_Type irq)
		{
			if (Scheduler::s_schedulerStarted)
				unRegisterIrqKernel(irq);
			else
				Scheduler::preSchedulerIrqUnregister(irq);
		}
		

		/*Setup an Irq Priority*/
		static inline void irqPriority(IRQn_Type irq, uint8_t preEmpt, uint8_t sub)
		{
			if(Scheduler::s_schedulerStarted)
				irqPriorityKernel(irq,preEmpt,sub);
			else
				IntVectManager::irqPriority(irq,preEmpt,sub);
		}

		static inline void irqPriority(IRQn_Type irq, uint8_t priority)
		{
			if(Scheduler::s_schedulerStarted)
				irqGlobalPriorityKernel(irq,priority);
			else
				IntVectManager::irqPriority(irq,priority);
		}

		/*Enable an Irq in NVIC*/
		static inline void enableIrq(IRQn_Type irq)
		{
			if(Scheduler::s_schedulerStarted)
			{
				asm volatile(
					"MOV R0,%0\n\t"
					"SVC %[immediate]"::"r" (irq), [immediate] "I"(ServiceCall::SvcNumber::enableIrq));
			}
			else
				IntVectManager::enableIrq(irq);
		}

		/*Disable an Irq in NVIC*/
		static inline void disableIrq(IRQn_Type irq)
		{
			if(Scheduler::s_schedulerStarted)
			{
				asm volatile(
					"MOV R0,%0\n\t"
					"SVC %[immediate]"::"r" (irq), [immediate] "I"(ServiceCall::SvcNumber::disableIrq));
			}
			else
				IntVectManager::disableIrq(irq);
		}

		static inline void clearIrq(IRQn_Type irq)
		{
			if(Scheduler::s_schedulerStarted)
			{
				asm volatile(
					"MOV R0,%0\n\t"
					"SVC %[immediate]"::"r" (irq), [immediate] "I"(ServiceCall::SvcNumber::clearIrq));
			}
			else
				IntVectManager::clearIrq(irq);
		}

	private:
		/*All Svc function are naked to keep Register with value stored when called*/
		static bool __attribute__((naked)) registerIrqKernel(IRQn_Type irq, IntVectManager::IrqHandler handler)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::registerIrq);
			asm volatile(
				"POP {LR}\n\t"
				"BX LR");
		}
		
		
		static bool __attribute__((naked)) unRegisterIrqKernel(IRQn_Type irq)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::unregisterIrq);
			asm volatile(
				"POP {LR}\n\t"
				"BX LR");
		}
		
		static void __attribute__((naked)) irqGlobalPriorityKernel(IRQn_Type irq, uint8_t priority)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::setGlobalPriority);
			asm volatile(
				"POP {LR}\n\t"
				"BX LR");
		}

		static void __attribute__((naked)) irqPriorityKernel(IRQn_Type irq, uint8_t preEmpt, uint8_t sub)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::setPriority);
			asm volatile(
				"POP {LR}\n\t"
				"BX LR");
		}

	};
	}
