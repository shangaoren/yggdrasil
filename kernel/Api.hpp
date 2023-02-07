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
#include "core/Core.hpp"
#include "Event.hpp"


namespace kernel
{
	class Api
	{
	public:
		
		/* Prepare Kernel by giving it system core reference and priority level */
		static inline void setupKernel(uint8_t kernelPriority = 0)
		{
			return Scheduler::setupKernel(kernelPriority);
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
#ifdef KDEBUG
		static inline void sleep(uint32_t ticks)
		{
			Y_ASSERT(Scheduler::inThreadMode());
			return core::Core::supervisorCall<ServiceCall::SvcNumber::sleepTask, void, uint32_t>(ticks);
		}
#else
		const static inline auto &sleep = core::Core::supervisorCall<ServiceCall::SvcNumber::sleepTask, void, uint32_t>;
#endif // KDEBUG

		

		/*get kernel timeStamp*/
		static inline uint64_t getTicks()
		{
			return Scheduler::s_ticks;
		}
		
		/*register an irq before the scheduler has started*/
		static void registerIrq(core::interfaces::Irq irq, core::interfaces::IVectorManager::IrqHandler handler, const char* name)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			registerIrqKernel(irq, handler,name);	
		}
		
		
		/*unregister an irq before the scheduler has started*/
		static void unregisterIrq(core::interfaces::Irq irq)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			unRegisterIrqKernel(irq);
		}
		

		/*Setup an Irq Priority*/
		static inline void irqPriority(core::interfaces::Irq irq, uint8_t preEmpt, uint8_t sub)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			irqPriorityKernel(irq,preEmpt,sub);
		}

		static inline void irqPriority(core::interfaces::Irq irq, uint8_t priority)
		{
			Y_ASSERT(priority >= Scheduler::s_systemPriority);
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			irqGlobalPriorityKernel(irq,priority);
		}

		/*Enable an Irq in NVIC*/
		static inline void enableIrq(core::interfaces::Irq irq)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			core::Core::supervisorCall < ServiceCall::SvcNumber::enableIrq, void, core::interfaces::Irq>(irq);
		}

		/*Disable an Irq in NVIC*/
		static inline void disableIrq(core::interfaces::Irq irq)
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installKernelInterrupt();
			core::Core::supervisorCall < ServiceCall::SvcNumber::disableIrq, void, core::interfaces::Irq>(irq);
		}

		/*Clear a pending Irq*/
		static inline void clearIrq(core::interfaces::Irq irq)
		{
			if (Scheduler::s_interruptInstalled)
				Scheduler::installKernelInterrupt();
			core::Core::supervisorCall < ServiceCall::SvcNumber::clearIrq, void, core::interfaces::Irq>(irq);
		}
		
		static inline void setupInterrupt(core::interfaces::Irq irq, core::interfaces::IVectorManager::IrqHandler handler, uint8_t priority, const char* name = nullptr)
		{
			registerIrq(irq, handler, name);
			irqPriority(irq, priority);
			clearIrq(irq);
			enableIrq(irq);
		}
		
		static inline void setupInterrupt(core::interfaces::Irq irq, core::interfaces::IVectorManager::IrqHandler handler, uint8_t preEmpt, uint8_t sub, const char* name = nullptr)
		{
			registerIrq(irq, handler, name);
			irqPriority(irq, preEmpt, sub);
			clearIrq(irq);
			enableIrq(irq);
		}
		
		/*Lock every interrupts below System*/
		static const inline auto& enterCriticalSection = core::Core::supervisorCall<ServiceCall::SvcNumber::enterCriticalSection, void>;
	
		/*Unlock Interrupts*/
		static const inline auto& exitCriticalSection = core::Core::supervisorCall<ServiceCall::SvcNumber::exitCriticalSection, void>;
	private:
		/* Register an interrupt */
		//@return true if success, false otherwise
		//@params irq to register, irqHandler to use when irq is triggered, const char* name of irq

		static const inline auto& registerIrqKernel = 
			core::Core::supervisorCall<ServiceCall::SvcNumber::registerIrq, bool, core::interfaces::Irq, core::interfaces::IVectorManager::IrqHandler, const char*>;
		
		// unregister an irq (will replace by default handler), the irq should not be called again
		//@return true if success, false otherwise
		//@params irq to unregister

		static const inline auto& unRegisterIrqKernel = 
			core::Core::supervisorCall < ServiceCall::SvcNumber::unregisterIrq, bool, core::interfaces::Irq>; 

		// set global irq Priority (ignore subpriority/ preempt splitting)
		//@return no return
		//@params irq to setup, priority to give

		static const inline auto& irqGlobalPriorityKernel = 
			core::Core::supervisorCall < ServiceCall::SvcNumber::setGlobalPriority,void, core::interfaces::Irq, uint8_t>;

		
		// set irq Priority whith priority and subpriority
		//@return no return
		//@params irq to setup, preempt priority, sub priority
		static const inline auto& irqPriorityKernel = 
			core::Core::supervisorCall < ServiceCall::SvcNumber::setPriority, void, core::interfaces::Irq, uint8_t, uint8_t>;
	};
	}
