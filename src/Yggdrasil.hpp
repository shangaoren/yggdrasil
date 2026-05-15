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
#include "framework/assert.hpp"
#include "kernel/Hooks.hpp"
#include "kernel/Scheduler.hpp"
#include "kernel/ServiceCall.hpp"
#include "YggdrasilConfig.hpp"


namespace kernel
{
	class Yggdrasil
	{
	public:

		/*wait without using kernel
		 *@Warning : will wait until time counter elapsed
		 *
		 */
		static void wait(const uint32_t ms)
		{
			const auto endWaitTimeStamp = Scheduler::getTicks() + ms;
			while (Scheduler::getTicks() <= endWaitTimeStamp) ;
		}

		/*set a Task into sleep for an amount of ms
		 *@Warning: do not call it if you're not in a Task*/
#ifdef KDEBUG
		static inline void sleep(const uint32_t ticks)
		{
			y_assert(Scheduler::inThreadMode());
			Core::SupervisorCallHelper<ServiceCall::SvcNumber::sleepTask, void(uint32_t)>::call(ticks);
		}
#else
		const static inline auto &sleep = Core::SupervisorCallHelper<ServiceCall::SvcNumber::sleepTask, void(uint32_t)>::call;
#endif // KDEBUG



		/*get kernel timeStamp*/
		static constexpr auto& getTicks = Scheduler::getTicks;

		static void init() {
			installKernelInterrupt();
		}

		static void start() {
			y_assert(!Scheduler::isStarted());
			y_assert(Vector::isInstalled());
			Core::idleTask.start(Core::idleFunc, true, 0, 0, "idle"); // Add idle task
			Core::SystemTimer::init(Core::getCoreFrequency(), Config::kSystemTimerFrequency);
			Core::SystemTimer::start();
			Hooks::onKernelStart();
			Core::template SupervisorCallHelper<ServiceCall::SvcNumber::startFirstTask,void()>::call();
			y_assert(true);
		}

		/*register an irq before the scheduler has started*/
		static void registerIrq(const core::Irq irq, Vector::IrqHandler handler, const char* name)
		{
			y_assert(Vector::isInstalled());
			registerIrqKernel(irq, handler,name);
		}


		/*unregister an irq before the scheduler has started*/
		static void unregisterIrq(const core::Irq irq)
		{
			y_assert(Vector::isInstalled());
			unRegisterIrqKernel(irq);
		}


		/*Setup an Irq Priority*/
		static inline void irqPriority(const core::Irq irq,const uint8_t preEmpt, const uint8_t sub)
		{
			y_assert(Vector::isInstalled());
			irqPriorityKernel(irq,preEmpt,sub);
		}

		static inline void irqPriority(const core::Irq irq, const uint8_t priority)
		{
			y_assert(Vector::isInstalled());
			irqGlobalPriorityKernel(irq,priority);
		}

		/*Enable an Irq in NVIC*/
		static inline void enableIrq(const core::Irq irq)
		{
			y_assert(Vector::isInstalled());
			Core::SupervisorCallHelper < ServiceCall::SvcNumber::enableIrq, void(core::Irq)>::call(irq);
		}

		/*Disable an Irq in NVIC*/
		static inline void disableIrq(const core::Irq irq)
		{
			y_assert(Vector::isInstalled());
			Core::SupervisorCallHelper< ServiceCall::SvcNumber::disableIrq, void(core::Irq)>::call(irq);
		}

		/*Clear a pending Irq*/
		static inline void clearIrq(const core::Irq irq)
		{
			y_assert(Vector::isInstalled());
			Core::SupervisorCallHelper < ServiceCall::SvcNumber::clearIrq, void(core::Irq)>::call(irq);
		}

		static inline void setupInterrupt(const core::Irq irq, Vector::IrqHandler handler,const uint8_t priority, const char* name = nullptr)
		{
			registerIrq(irq, handler, name);
			irqPriority(irq, priority);
			clearIrq(irq);
			enableIrq(irq);
		}

		static inline void setupInterrupt(const core::Irq irq, const Vector::IrqHandler handler, const uint8_t preEmpt, const uint8_t sub, const char* name = nullptr)
		{
			registerIrq(irq, handler, name);
			irqPriority(irq, preEmpt, sub);
			clearIrq(irq);
			enableIrq(irq);
		}

		/*Lock every interrupts below System*/
		static const inline auto& enterCriticalSection = Core::SupervisorCallHelper<ServiceCall::SvcNumber::enterCriticalSection, void()>::call;

		/*Unlock Interrupts*/
		static const inline auto& exitCriticalSection = Core::SupervisorCallHelper<ServiceCall::SvcNumber::exitCriticalSection, void()>::call;
	private:

		static bool installKernelInterrupt() {

			if (!Vector::install()) {
				return false;
			}
			//Setup Systick
			Vector::irqPriority(Core::systemTimerIrq, Config::kKernelPriority);
			Vector::registerHandler(Core::systemTimerIrq, Core::systemTimerHandler, "Systick");

			//Setup Supervisor Call interrupt
			Vector::irqPriority(Core::supervisorCallIrq, Config::kKernelPriority);
			Vector::registerHandler(Core::supervisorCallIrq, Core::supervisorCallHandler, "SVC");

			//Setup pendSV interrupt (used for task change)
			Vector::irqPriority(Core::taskSwitchIrq, 0xFF); //Minimum priority for task change
			Vector::registerHandler(Core::taskSwitchIrq, Core::contextSwitchHandler, "pendSV");
			return true;
		}
		/* Register an interrupt */
		//@return true if success, false otherwise
		//@params irq to register, irqHandler to use when irq is triggered, const char* name of irq

		static const inline auto& registerIrqKernel =
			Core::SupervisorCallHelper<ServiceCall::SvcNumber::registerIrq, bool(core::Irq, Vector::IrqHandler, const char*)>::call;

		// unregister an irq (will replace by default handler), the irq should not be called again
		//@return true if success, false otherwise
		//@params irq to unregister

		static const inline auto& unRegisterIrqKernel =
			Core::SupervisorCallHelper<ServiceCall::SvcNumber::unregisterIrq, bool(core::Irq)>::call;

		// set global irq Priority (ignore subpriority/ preempt splitting)
		//@return no return
		//@params irq to setup, priority to give

		static const inline auto& irqGlobalPriorityKernel =
			Core::SupervisorCallHelper<ServiceCall::SvcNumber::setGlobalPriority, void(core::Irq, uint8_t)>::call;


		// set irq Priority whith priority and subpriority
		//@return no return
		//@params irq to setup, preempt priority, sub priority
		static const inline auto& irqPriorityKernel =
			Core::SupervisorCallHelper<ServiceCall::SvcNumber::setPriority, void(core::Irq, uint8_t, uint8_t)>::call;
	};
	}
