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


#include "Task.hpp"
#include "ServiceCall.hpp"
#include "Event.hpp"
#include "Mutex.hpp"

#include "yggdrasil/interfaces/IVectorsManager.hpp"
#include "yggdrasil/kernel/Core.hpp"
#include "yggdrasil/framework/DualLinkedList.hpp"
#include "yggdrasil/framework/Assertion.hpp"



namespace kernel
{
	using namespace core::interfaces;
	class Scheduler
	{
		friend class ServiceCall;
		friend class Api;
		friend class SystemView;
		friend class Task;
		friend class Mutex;
		friend class Event;
	private:
		
		
		enum class changeTaskTrigger : uint32_t
		{
			enterSleep = 0,
			exitSleep = 1,
			waitForEvent = 2,
			wakeByEvent = 3,
			taskStarted = 4,
			taskStopped = 5,
			waitForMutex = 6,
			wakeByMutex = 7,
			none = 20,
		};
		
		static void setupKernel(Core& systemCore, uint8_t systemPriority)
		{
			s_core = &systemCore;
			s_systemPriority = systemPriority;
		}
		
		static bool startKernel()
		{
			if (s_schedulerStarted)	//Scheduler already started 
				return false;
			if (s_ready.isEmpty()) //No Task in task list, cannot start Scheduler
				return false;
			
			s_idle.start();	//Add idle task
			
			if(!installKernelInterrupt())
				return false;
			
			//Init Systick
			s_core->systemTimer().initSystemTimer(s_core->coreClocks().getSystemCoreFrequency(), 1000);
			s_core->systemTimer().startSystemTimer();

			svc(ServiceCall::SvcNumber::startFirstTask);	//Switch to service call to start the first task
			__BKPT(0);	//shouldn't end here
			return true;
		}
		
		static bool installKernelInterrupt()
		{
			if (s_interruptInstalled)
				return true;
			Y_ASSERT(s_core != nullptr);
			// Install vector manager if not already done 
			if(!s_core->vectorManager().isInstalled())
			{
				if (!s_core->vectorManager().installVectorManager())
					return false;
			}
			
			//Setup Systick
			s_core->vectorManager().irqPriority(s_core->systemTimer().getIrq(), s_systemPriority);
			s_core->vectorManager().registerHandler(s_core->systemTimer().getIrq(), systickHandler);
			
			//Setup Service Call interrupt 
			s_core->vectorManager().irqPriority(SVCall_IRQn, s_systemPriority + 1);
			s_core->vectorManager().registerHandler(SVCall_IRQn, svcBootstrap);
			
			//Setup pendSV interrupt (used for task change)
			s_core->vectorManager().irqPriority(PendSV_IRQn, 0xFF); 	//Minimum priority for pendsv (task change)
			s_core->vectorManager().registerHandler(PendSV_IRQn, pendSvHandler);
			s_interruptInstalled = true;
			return true;
		}

		static bool __attribute__((aligned(4), optimize("O0"))) startFirstTask()
		{
			//start a task, reset main stack pointer
			s_activeTask = s_ready.getFirst();
			s_schedulerStarted = true;
			asm volatile("MOV R0,%0" //load stack pointer from task.stackPointer
						 ::"r"(s_activeTask->m_stackPointer));
			asm volatile("LDMIA R0!,{R2-R11}");//restore registers R4 to R11
			asm volatile("MOV LR,R2");//reload Link register
			asm volatile("MSR CONTROL,R3");//reload CONTROL register
			asm volatile("ISB");//Instruction synchronisation Barrier is recommended after changing CONTROL
			asm volatile("MSR PSP,R0"); //reload process stack pointer with task's stack pointer
			//__set_MSP(_estack); //reset main stack pointer to save space (main is now useless)
			asm volatile("BX LR"); //branch to task
			return true; //should never return here
	}
		
		/*****************************************************DATA*****************************************************/
		
		/* Interrupts Variables */
		static Core* s_core;
		static uint8_t s_systemPriority;
		static bool s_interruptInstalled;
		
		/* Tasks Lists */
		static ReadyList s_ready;
		static SleepingList s_sleeping;
		static StartedList s_started;
		
		/* Task Related Variables */
		volatile static changeTaskTrigger s_trigger;
		static Task* volatile s_activeTask;
		static Task* volatile s_previousTask;
		static uint32_t s_sysTickFreq;
		
		/* Scheduler misc */
		static bool s_schedulerStarted;
		volatile static uint64_t s_ticks;
		static TaskWithStack<256> s_idle;
		
		
		
		
		/****************************************************FUNCTIONS*************************************************/

		
		//register an Irq, only accessed via service call 
		static inline bool IrqRegister(IVectorManager::Irq irq, core::interfaces::IVectorManager::IrqHandler handler)
		{
			s_core->vectorManager().registerHandler(irq, handler);
			return true;
		}
		
		
		//unregister an Irq, only accessed via service call
		static inline bool IrqUnregister(IVectorManager::Irq irq)
		{
			s_core->vectorManager().unregisterHandler(irq);
			return true;
		}
		
		
		/*Lock all interrupt lower or equal of system*/
		static inline void enterKernelCriticalSection()
		{
			s_core->vectorManager().lockAllInterrupts();
		}
		
		/*release Interrupt lock*/
		static inline void exitKernelCriticalSection()
		{
			s_core->vectorManager().enableAllInterrupts();
		}
		
		//start a task
		static bool startTask(Task& task)
		{
			if (task.m_started)
				return false;		
			enterKernelCriticalSection();
			s_started.insert(&task, Task::priorityCompare);
			s_ready.insert(&task, Task::priorityCompare);
			exitKernelCriticalSection();
			task.m_started = true;
			task.m_state = Task::state::ready;
			if (s_schedulerStarted)
				schedule(kernel::Scheduler::changeTaskTrigger::taskStarted);
			return true;
		}
		
		/**
		 * Look at ready task to see if a context switching is needed
		 ***/
		static bool schedule(changeTaskTrigger trigger)
		{
			enterKernelCriticalSection();
			Y_ASSERT(s_activeTask != nullptr);
			Y_ASSERT(s_ready.count() != 0); //assertion to check there is ready tasks
			if (s_ready.peekFirst()->m_taskPriority > s_activeTask->m_taskPriority) //a task with higher priority is waiting, trigger context switching
			{
				//Store currently running task
				Y_ASSERT(!s_ready.contain(s_activeTask)); //currently running task not already in ready list
				s_ready.insert(s_activeTask, Task::priorityCompare);
				
				
				//If there is no task to be saved already, put active task in s_previousTask to save context
				if(s_previousTask == nullptr)
					s_previousTask = s_activeTask;
				
				//Take the first available task
				s_activeTask = s_ready.getFirst();
				Y_ASSERT(s_activeTask != nullptr);
				exitKernelCriticalSection();
				setPendSv(trigger);
				return true;
			}
			else
			{
				exitKernelCriticalSection();
				return false;
			}
		}
		

		/*Stop a Task*/
		/*TODO Implement*/
		static bool stopTask(Task& task)
		{
			enterKernelCriticalSection();
			s_ready.remove(&task);
			s_sleeping.remove(&task);
			s_started.remove(&task);
			if (s_activeTask == &task)
			{
				s_activeTask = s_ready.getFirst();	
				s_activeTask->m_state = Task::state::active;
				asm volatile(
					"MOV R0,%0\n\t"		//load stack pointer from task.stackPointer
					"LDMIA R0!,{R2-R11}\n\t"	//restore registers R4 to R11
					"MOV LR,R2\n\t"		//reload Link register
					"MSR CONTROL,R3\n\t"//reload CONTROL register
					"ISB\n\t"				//Instruction synchronisation Barrier is recommended after changing CONTROL
					"MSR PSP,R0\n\t"	//reload process stack pointer with task's stack pointer
					"BX LR"
					::"r" (s_activeTask->m_stackPointer));
			}
			exitKernelCriticalSection();
			return true;
		}
		
		
		//function to sleep a task for a number of ms
		static bool sleep(uint32_t ms)
		{
			s_activeTask->m_wakeUpTimeStamp = s_ticks + ms;
			enterKernelCriticalSection();

			//Put active Task to sleep
			Y_ASSERT(!s_sleeping.contain(s_activeTask));  // if active task already in sleeping list we have a problem
			s_sleeping.insert(s_activeTask, Task::sleepCompare);
			//s_previous task should be empty since no context switch should be pending
			Y_ASSERT(s_previousTask == nullptr);
			s_previousTask = s_activeTask;
			
			//Get the first available ready task
			s_activeTask = s_ready.getFirst();
			Y_ASSERT(s_activeTask != nullptr);  // ready list should at least contain idle task
			s_previousTask->m_state = Task::state::sleeping;
			exitKernelCriticalSection();
			setPendSv(kernel::Scheduler::changeTaskTrigger::enterSleep);  //active task is sleeping, trigger context switch
			return true;
		}
		
		
		//Handler of pendsv exception, act as context switcher
		static void __attribute__((naked, optimize("O0"))) __attribute__((aligned(4))) pendSvHandler()
		{
			uint32_t* stackPosition = nullptr;
			asm volatile(
				"CPSID I\n\t"	//set primask (disable all interrupts)
				"MRS R0, PSP\n\t"	//store Process Stack pointer into R0
				"MOV R2,LR\n\t"		//store Link Register into R2
				"MRS R3,CONTROL\n\t"//store CONTROL register into R3
				"STMDB R0!,{R2-R11}\n\t" //store R4 to R11 memory pointed by R0 (stack), increment memory and rewrite the new adress to R0
				"MOV %0,R0" //store stack pointer in task.stackPointer
				///"PUSH {LR}"	//save link return to be able to return later
				: "=r" (stackPosition));
			changeTask(stackPosition);
			asm volatile(
				"MOV R0,%0\n\t"		//load stack pointer from task.stackPointer
				"LDMIA R0!,{R2-R11}\n\t"	//restore registers R4 to R11
				"MOV LR,R2\n\t"		//reload Link register
				"MSR CONTROL,R3\n\t"//reload CONTROL register
				"ISB\n\t"				//Instruction synchronisation Barrier is recommended after changing CONTROL
				"MSR PSP,R0\n\t"	//reload process stack pointer with task's stack pointer
				"CPSIE I\n\t"	//clear primask (enable interrupts)
				::"r" (s_activeTask->m_stackPointer));
			asm volatile("BX LR");
		}
		
		
		static void changeTask(uint32_t* stackPosition)
		{
			//No Task to switch, error
			Y_ASSERT(s_activeTask != nullptr);		
			if(!(s_previousTask == nullptr)) // avoid Spurious interrupt
			{
				s_previousTask->m_stackPointer = stackPosition;    //save stackPosition
				s_previousTask = nullptr; //Context change done no need to know previous task anymore
				s_activeTask->m_state = kernel::Task::state::active;
				s_trigger = kernel::Scheduler::changeTaskTrigger::none;
			}
			else
			{
				Y_ASSERT(false); //trick to get breakpoint only when debug
				s_activeTask->m_stackPointer = stackPosition;
			}
		}
		
		
		//set pendSv, trigger context switch
		static void setPendSv(changeTaskTrigger trigger)
		{
			s_trigger = trigger;
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
		

		//systick handler 
		static void systickHandler()
		{
			bool needSchedule = false;
			s_ticks++;
			while (!s_sleeping.isEmpty() && s_sleeping.peekFirst()->m_wakeUpTimeStamp <= s_ticks) //one task or more is waiting, let's see if waiting is over
			{
				needSchedule = true;
				enterKernelCriticalSection();
				Task* readyTask = s_sleeping.getFirst();
				readyTask->m_wakeUpTimeStamp = 0;
				readyTask->m_state = kernel::Task::state::ready;
				Y_ASSERT(!s_ready.contain(readyTask)); //new Ready task should not being already in ready list
				s_ready.insert(readyTask, Task::priorityCompare);
				exitKernelCriticalSection();
			}
			if (needSchedule)
				schedule(kernel::Scheduler::changeTaskTrigger::exitSleep);
			
		}
		
		
		static void __attribute__((aligned(4))) __attribute__((naked, optimize("O0"))) svcBootstrap()
		{
			uint32_t* stackedPointer;
			asm volatile(
				"TST LR,#4\n\t" //test bit 2 of EXC_RETURN to know if MSP or PSP 
				"ITE EQ\n\t"	//was used for stacking
				"MRSEQ R0, MSP\n\t"		//place msp or psp in R0 as parameter for SvcHandler
				"MRSNE R0, PSP\n\t" 
				"MOV %0,R0\n\t" : "=r" (stackedPointer) :);
			asm volatile("PUSH {LR}\n\t");    //stack LR to be able to return for exception
			svcHandler(stackedPointer);
			asm volatile("POP {PC}");   	//return from exception
		}
		
		
		
		//Svc handler, redirect svc call to the right function
		static void __attribute__((aligned(4))) svcHandler(uint32_t* args)
		{
			ServiceCall::SvcNumber askedService;
			uint32_t param0 = args[0], param1 = args[1], param2 = args[2];
			//uint32_t& pc = *reinterpret_cast<uint32_t*>(args[6]);
			askedService = (reinterpret_cast<ServiceCall::SvcNumber*>(args[6]))[-2];  //get stacked PC, then -2 to get SVC instruction 
			
			switch (askedService)
			{
			case ServiceCall::SvcNumber::startFirstTask:
				args[0] = startFirstTask();     //write result to stacked R0
				break;
				
			case ServiceCall::SvcNumber::registerIrq:
				args[0] = IrqRegister(static_cast<IVectorManager::Irq>(param0), reinterpret_cast<core::interfaces::IVectorManager::IrqHandler>(param1));
				break;
				
			case ServiceCall::SvcNumber::unregisterIrq:
				args[0] = IrqUnregister(static_cast<IVectorManager::Irq>(param0)); 	//execute function and write result to stacked R0
				break;
				
			case ServiceCall::SvcNumber::startTask:
				args[0] = startTask(*(reinterpret_cast<Task*>(param0)));
				break;
				
			case ServiceCall::SvcNumber::stopTask:
				args[0] = stopTask(*(reinterpret_cast<Task*>(param0)));
				break;
				
			case ServiceCall::SvcNumber::sleepTask:
				args[0] = sleep(param0);
				break;
				
			case ServiceCall::SvcNumber::signalEvent:
				Event::kernelSignalEvent(reinterpret_cast<Event*>(param0));
				break;
				
			case ServiceCall::SvcNumber::waitEvent:
				Event::kernelWaitEvent(reinterpret_cast<Event*>(param0));
				break;
				
			case ServiceCall::SvcNumber::enableIrq:
				s_core->vectorManager().enableIrq(static_cast<IVectorManager::Irq>(param0));
				break;

			case ServiceCall::SvcNumber::disableIrq:
				s_core->vectorManager().disableIrq(static_cast<IVectorManager::Irq>(param0));
				break;

			case ServiceCall::SvcNumber::clearIrq:
				s_core->vectorManager().clearIrq(static_cast<IVectorManager::Irq>(param0));
				break;

			case ServiceCall::SvcNumber::setGlobalPriority:
				s_core->vectorManager().irqPriority(static_cast<IVectorManager::Irq>(param0), static_cast<uint8_t>(param1));
				break;

			case ServiceCall::SvcNumber::setPriority:
				s_core->vectorManager().irqPriority(static_cast<IVectorManager::Irq>(param0), static_cast<uint8_t>(param1), static_cast<uint8_t>(param2));
				break;
			case ServiceCall::SvcNumber::enterCriticalSection:
				s_core->vectorManager().lockInterruptsLowerThan(s_systemPriority - 2);
				break;
				
			case ServiceCall::SvcNumber::exitCriticalSection:
				s_core->vectorManager().lockInterruptsLowerThan(0); 	//set basepri to 0 in order to enable all interrupts 
				break;
					
			case kernel::ServiceCall::SvcNumber::mutexLock:
				args [0] = Mutex::kernelLockMutex(reinterpret_cast<Mutex*>(param0), param1);
				break;
			case kernel::ServiceCall::SvcNumber::mutexRelease:
				args[0] = Mutex::kernelReleaseMutex(reinterpret_cast<Mutex*>(param0));
				break;

			default:	//unknown Service call number
				__BKPT(0);
				break;
			}
		}
		
		//Function of Idle Task (NOP when NDEBUG is defined, WFI when release)
		static void idleTaskFunction(uint32_t param)
		{
			do
			{
#ifdef NDEBUG
				__NOP();
#else
				__WFI();
#endif
			} while (true);
		}		
		
		static uint64_t getTicks()
		{
			return s_ticks;
		}
		
	};
	}
