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
#include "Scheduler.hpp"
#include "ServiceCall.hpp"
#include "yggdrasil/interfaces/IVectorsManager.hpp"
#include "yggdrasil/kernel/Core.hpp"
#include "Event.hpp"


namespace kernel
{
	class Api
	{
	public:
		
		/* Prepare Kernel by giving it system core reference and priority level */
		static inline void setupKernel(Core& systemCore, uint8_t kernelPriority = 0)
		{
			return Scheduler::setupKernel(systemCore, kernelPriority);
		}
		
		/* Start Kernel*/
		static inline bool startKernel()
		{
			return Scheduler::startKernel();
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
			Scheduler::checkStack();
			asm volatile(
				"MOV R0,%0\n\t"
				"SVC %[immediate]"::"r" (ms), [immediate] "I"(ServiceCall::SvcNumber::sleepTask));
			Scheduler::checkStack();
		}
		
		
		/*get kernel timeStamp*/
		static inline uint64_t getTicks()
		{
			return Scheduler::s_ticks;
		}
		
		/*register an irq before the scheduler has started*/
		static void registerIrq(core::interfaces::IVectorManager::Irq irq, core::interfaces::IVectorManager::IrqHandler handler, const char* name)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			registerIrqKernel(irq, handler,name);	
		}
		
		
		/*unregister an irq before the scheduler has started*/
		static void unregisterIrq(core::interfaces::IVectorManager::Irq irq)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			unRegisterIrqKernel(irq);
		}
		

		/*Setup an Irq Priority*/
		static inline void irqPriority(core::interfaces::IVectorManager::Irq irq, uint8_t preEmpt, uint8_t sub)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			irqPriorityKernel(irq,preEmpt,sub);
		}

		static inline void irqPriority(core::interfaces::IVectorManager::Irq irq, uint8_t priority)
		{
			Y_ASSERT(priority >= Scheduler::s_systemPriority);
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			irqGlobalPriorityKernel(irq,priority);
		}

		/*Enable an Irq in NVIC*/
		static inline __attribute__((optimize("O0"))) void enableIrq(core::interfaces::IVectorManager::Irq irq)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			asm volatile(
					"MOV R0,%0\n\t"
					"SVC %[immediate]"::"r" (irq), [immediate] "I"(ServiceCall::SvcNumber::enableIrq));
		}

		/*Disable an Irq in NVIC*/
		static inline __attribute__((optimize("O0"))) void disableIrq(core::interfaces::IVectorManager::Irq irq)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			asm volatile(
					"MOV R0,%0\n\t"
					"SVC %[immediate]"::"r" (irq), [immediate] "I"(ServiceCall::SvcNumber::disableIrq));
		}

		/*Clear a pending Irq*/
		static inline __attribute__((optimize("O0"))) void clearIrq(core::interfaces::IVectorManager::Irq irq)
		{
			if (Scheduler::s_interruptInstalled)
				Scheduler::installKernelInterrupt();
			asm volatile(
					"MOV R0,%0\n\t"
					"SVC %[immediate]"::"r" (irq), [immediate] "I"(ServiceCall::SvcNumber::clearIrq));
		}
		
		static inline void setupInterrupt(core::interfaces::IVectorManager::Irq irq, core::interfaces::IVectorManager::IrqHandler handler, uint8_t priority, const char* name = nullptr)
		{
			registerIrq(irq, handler, name);
			irqPriority(irq, priority);
			clearIrq(irq);
			enableIrq(irq);
		}
		
		static inline void setupInterrupt(core::interfaces::IVectorManager::Irq irq, core::interfaces::IVectorManager::IrqHandler handler, uint8_t preEmpt, uint8_t sub, const char* name = nullptr)
		{
			registerIrq(irq, handler, name);
			irqPriority(irq, preEmpt, sub);
			clearIrq(irq);
			enableIrq(irq);
		}
		
		/*Lock every interrupts below System*/
		static void __attribute__((naked, optimize("O0"))) enterCriticalSection()
		{
			Scheduler::checkStack();
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::enterCriticalSection);
			asm volatile("POP {PC}");
			Scheduler::checkStack();
		}
		
		/*Unlock Interrupts*/
		static void __attribute__((naked)) exitCriticalSection()
		{
			Scheduler::checkStack();
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::exitCriticalSection);
			asm volatile("POP {PC}");
			Scheduler::checkStack();
		}

	private:
		/*All Svc function are naked to keep Register with value stored when called*/
		static bool __attribute__((naked)) registerIrqKernel(core::interfaces::IVectorManager::Irq irq, core::interfaces::IVectorManager::IrqHandler handler, const char* name)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::registerIrq);
			asm volatile("POP {PC}");
		}
		
		
		static bool __attribute__((naked)) unRegisterIrqKernel(core::interfaces::IVectorManager::Irq irq)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::unregisterIrq);
			asm volatile("POP {PC}");
		}
		
		static void __attribute__((naked)) irqGlobalPriorityKernel(core::interfaces::IVectorManager::Irq irq, uint8_t priority)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::setGlobalPriority);
			asm volatile("POP {PC}");
		}

		static void __attribute__((naked)) irqPriorityKernel(core::interfaces::IVectorManager::Irq irq, uint8_t preEmpt, uint8_t sub)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::setPriority);
			asm volatile("POP {PC}");
		}
	};
	}
