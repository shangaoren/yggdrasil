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
#include "IntVectManager.hpp"
#include "Event.hpp"
#include "Processor.hpp"
#include "yggdrasil/framework/DualLinkedList.hpp"
#include "yggdrasil/interfaces/ISystem.hpp"

#ifdef SYSVIEW
#include "yggdrasil/systemview/segger/SEGGER_SYSVIEW.h"
#endif

extern uint32_t _estack;


namespace kernel
{
	class Scheduler
	{
		friend class ServiceCall;
		friend class Api;
		friend class SystemView;
		friend class Task;
	private:
		
		static bool startKernel(interfaces::ISystem& system, uint8_t systemInterruptPriority, uint8_t numberOfSubBits)
		{
			if (s_schedulerStarted)	//Scheduler already started 
<<<<<<< Updated upstream
				return false;
			if (s_ready.isEmpty()) //No Task in task list, cannot start Scheduler
				return false;
			
			s_idle.start();	//Add idle task

			s_systemInterface = &system;
			if(!s_systemInterface->initSystemClock()) //Attempt to start system clock 
				return false;
			
			s_subPriorityBits = numberOfSubBits;	//Sets Number of subPriority bits for interrupts
			s_systemPriority = systemInterruptPriority; //Sets Priority of System

			if (!s_interruptInstalled)	//Install interrupt if not already installed
				installSystemInterrupts();
			systickInit();	//init systick

			svc(ServiceCall::SvcNumber::startFirstTask);	//Switch to service call to start the first task
			__BKPT(0);	//shouldn't end here
			return true;
		}
		
		static bool startKernel(interface::ISystem& system)
		{
			if (s_schedulerStarted)	//Scheduler already started
				return false;
			if (s_ready.isEmpty())	//No Task in task list, cannot start Scheduler
				return false;
			
			s_idle.start(); 	//Add idle task

			s_systemInterface = &system;
			if (!s_systemInterface->initSystemClock())	//Attempt to start system clock
=======
				return false;
			if (s_ready.isEmpty()) //No Task in task list, cannot start Scheduler
				return false;
			
			s_idle.start();	//Add idle task

			s_systemInterface = &system;
			if(!s_systemInterface->initSystemClock()) //Attempt to start system clock 
>>>>>>> Stashed changes
				return false;
			
			s_subPriorityBits = numberOfSubBits;	//Sets Number of subPriority bits for interrupts
			s_systemPriority = systemInterruptPriority; //Sets Priority of System

<<<<<<< Updated upstream
			if(!s_interruptInstalled)	//Install system interrupts if not already installed
				installSystemInterrupts();
			systickInit();	//Init systick

			svc(ServiceCall::SvcNumber::startFirstTask);	//Switch to service call to start the first task
			__BKPT(0);	//shouldn't end here
=======
			if (!s_interruptInstalled)	//Install interrupt if not already installed
				installSystemInterrupts();
			systickInit();	//init systick

			svc(ServiceCall::SvcNumber::startFirstTask);	//Switch to service call to start the first task
			__BKPT(0);	//shouldn't end here
			return true;
		}
		
		static bool startKernel(interfaces::ISystem& system)
		{
			if (s_schedulerStarted)	//Scheduler already started
				return false;
			if (s_ready.isEmpty())	//No Task in task list, cannot start Scheduler
				return false;
			
			s_idle.start(); 	//Add idle task

			s_systemInterface = &system;
			if (!s_systemInterface->initSystemClock())	//Attempt to start system clock
				return false;

			if(!s_interruptInstalled)	//Install system interrupts if not already installed
				installSystemInterrupts();
			systickInit();	//Init systick

			svc(ServiceCall::SvcNumber::startFirstTask);	//Switch to service call to start the first task
			__BKPT(0);	//shouldn't end here
>>>>>>> Stashed changes
			return true;
		}
		
		static void installSystemInterrupts()
		{
			//set vectorTable
			#ifdef SYSVIEW
			sysviewInitInterruptStub();
			SCB->VTOR = s_sysviewVectorTable.tableBaseAddress();
			#else
			SCB->VTOR = s_vectorTable.tableBaseAddress();
			#endif

			//configure severe interrupts
			s_vectorTable.registerHandler(HardFault_IRQn, hardFault);
			s_vectorTable.registerHandler(NonMaskableInt_IRQn, nmi);
			s_vectorTable.registerHandler(UsageFault_IRQn, usageFault);
			s_vectorTable.registerHandler(BusFault_IRQn, busFault);

			/*Configure System Interrupts*/
			s_vectorTable.subPriorityBits(s_subPriorityBits);


			//configure service call
			s_vectorTable.registerHandler(SVCall_IRQn, svcBootstrap);
			s_vectorTable.irqPriority(SVCall_IRQn, (s_systemPriority + 1));


			//configure pendSv
			s_vectorTable.registerHandler(PendSV_IRQn, pendSvHandler);
			s_vectorTable.irqPriority(PendSV_IRQn, 0xFF);


			//configure systick
			s_vectorTable.registerHandler(SysTick_IRQn, systickHandler);
			s_vectorTable.irqPriority(SysTick_IRQn, s_systemPriority);
			s_interruptInstalled = true;
		}

		static bool __attribute__((aligned(4), , optimize("O0"))) startFirstTask()
		{
			//start a task, reset main stack pointer
			s_activeTask = s_ready.getFirst();
			s_schedulerStarted = true;
			asm volatile(
					"MOV R0,%0\n\t"		//load stack pointer from task.stackPointer
					"LDMIA R0!,{R2-R11}\n\t"//restore registers R4 to R11
					"MOV LR,R2\n\t"//reload Link register
					"MSR CONTROL,R3\n\t"//reload CONTROL register
					"ISB\n\t"//Instruction synchronisation Barrier is recommended after changing CONTROL
					"MSR PSP,R0\n\t"//reload process stack pointer with task's stack pointer
					::"r" (s_activeTask->m_stackPointer));
			//__set_MSP(_estack); //reset main stack pointer to save space (main is now useless)
			asm volatile("BX LR"); //branch to task
			return true; //should never return here
	}
		
		/*****************************************************DATA*****************************************************/
		
#ifdef SYSVIEW
		static IntVectManager s_sysviewVectorTable;
#endif
		static IntVectManager s_vectorTable;
		static interfaces::ISystem* s_systemInterface;
		static bool s_interruptInstalled;
		static uint8_t s_systemPriority;
		static uint8_t s_subPriorityBits;
		
		static ReadyList s_ready;
		static SleepingList s_sleeping;
		static StartedList s_started;
		
		
		static bool s_schedulerStarted;
		volatile static uint64_t s_ticks;
		
		static Task* volatile s_activeTask;
		static Task* volatile s_previousTask;
		static uint32_t s_sysTickFreq;
		
		static TaskWithStack<256> s_idle;
		
		
		
		
		/****************************************************FUNCTIONS*************************************************/

		
		//register an Irq, only accessed via service call 
		static inline bool IrqRegister(IRQn_Type irq, IntVectManager::IrqHandler handler)
		{
			s_vectorTable.registerHandler(irq, handler);
			return true;
		}
		
		
		//unregister an Irq, only accessed via service call
		static inline bool IrqUnregister(IRQn_Type irq)
		{
			s_vectorTable.unregisterHandler(irq);
			return true;
		}
		
		
		/*Lock all interrupt lower or equal of system*/
		static inline void enterKernelCriticalSection()
		{
			IntVectManager::lockAllInterrupts();
		}
		
		/*release Interrupt lock*/
		static inline void exitCriticalSection()
		{
			IntVectManager::enableAllInterrupts();
		}
		
		//start a task
		static bool startTask(Task& task)
		{
			if (task.m_started)
				return false;
			
#ifdef SYSVIEW
			SEGGER_SYSVIEW_OnTaskCreate(reinterpret_cast<uint32_t>(&task));
			SEGGER_SYSVIEW_SendTaskInfo(&task.m_info);
			SEGGER_SYSVIEW_OnTaskStartReady(reinterpret_cast<uint32_t>(&task));
#endif		
			enterKernelCriticalSection();
			s_started.insert(&task, Task::priorityCompare);
			s_ready.insert(&task, Task::priorityCompare);
			exitCriticalSection();
			task.m_started = true;
			task.m_state = Task::state::ready;
			if (s_schedulerStarted)
				schedule();
			return true;
		}
		
		/**
		 * Look at ready task to see if a context switching is needed
		 ***/
		static bool schedule()
		{
			if (s_ready.peekFirst()->m_taskPriority > s_activeTask->m_taskPriority) //a task with higher priority is waiting, trigger context switching
			{
				enterKernelCriticalSection();
				
				//Store currently running task
				if (s_ready.contain(s_activeTask))
					__BKPT(0);
				if (!s_ready.insert(s_activeTask, Task::priorityCompare))
					__BKPT(0);
				
				//If there is no task to be saved already, put active task in s_previousTask to save context
				if(s_previousTask == nullptr)
					s_previousTask = s_activeTask;
				
				//Take the first available task
				s_activeTask = s_ready.getFirst();
				if (s_activeTask == nullptr)
					__BKPT(0);
				
				exitCriticalSection();
				setPendSv();
				return true;
			}
			else
				return false;
		}
		
		
		//Task rise an event
		static bool signalEvent(Event* event)
		{
			if (!event->m_list.isEmpty())
			{
				Task* newReadyTask = event->m_list.getFirst();
#ifdef SYSVIEW
				SEGGER_SYSVIEW_OnTaskStartReady(reinterpret_cast<uint32_t>(newReadyTask));
#endif
				enterKernelCriticalSection();
				if (s_ready.contain(newReadyTask))
					__BKPT(0);
				if (!s_ready.insert(newReadyTask, Task::priorityCompare))
					__BKPT(0);
				exitCriticalSection();
				schedule();
			}
			else
				event->m_isRaised = true;
			return true;
		}
		
		
		//A task ask to wait for an event
		static bool waitEvent(Event* event)
		{
			if (event->m_isRaised)	//event already rised, return
			{
				event->m_isRaised = false;
				return true;
			}
			else	//add task at the end of the waiting list
			{
				enterKernelCriticalSection();
#ifdef SYSVIEW
				SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask), 0);  //TODO add cause
#endif
				event->m_list.insert(s_activeTask, Task::priorityCompare);
				s_activeTask->m_state = Task::state::waiting;
				if(s_previousTask == nullptr)
					s_previousTask = s_activeTask;
				s_activeTask = s_ready.getFirst();
				if(s_activeTask == nullptr)
					__BKPT(0);
				setPendSv();
				exitCriticalSection();
			}
			return true;
		}
		
		
		/*Stop a Task*/
		/*TODO Implement*/
		static bool stopTask(Task& task)
		{
#ifdef SYSVIEW
			SEGGER_SYSVIEW_OnTaskStopExec();
#endif
			enterKernelCriticalSection();
			s_ready.remove(&task);
			s_sleeping.remove(&task);
			s_started.remove(&task);
			if (s_activeTask == &task)
			{
				s_activeTask = s_ready.getFirst();	
#ifdef SYSVIEW
				SEGGER_SYSVIEW_OnTaskStopExec();
				SEGGER_SYSVIEW_OnTaskStartExec(reinterpret_cast<uint32_t>(s_activeTask));
#endif
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
			exitCriticalSection();
			return true;
		}
		
		
		//function to sleep a task for a number of ms
		static bool sleep(uint32_t ms)
		{
			s_activeTask->m_wakeUpTimeStamp = s_ticks + ms;	
#ifdef SYSVIEW
			SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask),0);
#endif
			enterKernelCriticalSection();
			
			//Put active Task to sleep
			if (s_sleeping.contain(s_activeTask))
				__BKPT(0);
<<<<<<< Updated upstream
			if (!s_sleeping.insert(s_activeTask, Task::priorityCompare))
=======
			if (!s_sleeping.insert(s_activeTask, Task::sleepCompare))
>>>>>>> Stashed changes
				__BKPT(0);
			
			//s_previous task should be empty since no context switch should be pending
			if(s_previousTask != nullptr)
				__BKPT(0);
			else
				s_previousTask = s_activeTask;
			
			//Get the first available ready task
			s_activeTask = s_ready.getFirst();
			if (s_activeTask == nullptr)
				__BKPT(0);
			s_previousTask->m_state = Task::state::sleeping;
			exitCriticalSection();
			setPendSv(); //active task is sleeping, trigger context switch
			return true;
		}
		
		
		
		
		
		//Handler of pendsv exception, act as context switcher
		static void __attribute__((naked, optimize("O0"))) __attribute__((aligned(4))) pendSvHandler()
		{
			uint32_t* stackPosition = nullptr;
			asm volatile(
				"MRS R0, PSP\n\t"	//store Process Stack pointer into R0
				"MOV R2,LR\n\t"		//store Link Register into R2
				"MRS R3,CONTROL\n\t"//store CONTROL register into R3
				"STMDB R0!,{R2-R11}\n\t" //store R4 to R11 memory pointed by R0 (stack), increment memory and rewrite the new adress to R0
				"MOV %0,R0\n\t" //store stack pointer in task.stackPointer
				"PUSH {LR}"	//save link return to be able to return later
				: "=r" (stackPosition));
			enterKernelCriticalSection();
			changeTask(stackPosition);
			asm volatile(
				"MOV R0,%0\n\t"		//load stack pointer from task.stackPointer
				"LDMIA R0!,{R2-R11}\n\t"	//restore registers R4 to R11
				"MOV LR,R2\n\t"		//reload Link register
				"MSR CONTROL,R3\n\t"//reload CONTROL register
				"ISB\n\t"				//Instruction synchronisation Barrier is recommended after changing CONTROL
				"MSR PSP,R0\n\t"	//reload process stack pointer with task's stack pointer
				::"r" (s_activeTask->m_stackPointer));
			exitCriticalSection();
			asm volatile("POP { PC }");
		}
		
		
		static void changeTask(uint32_t* stackPosition)
		{
			//No Task to switch, error
			if((s_previousTask == nullptr) || (s_activeTask == nullptr))
				__BKPT(0);
			s_previousTask->m_stackPointer = stackPosition;    //save stackPosition
			
#ifdef SYSVIEW
			SEGGER_SYSVIEW_OnTaskStopExec();
			switch (s_activeTask->m_state)
			{
			case kernel::Task::state::sleeping:
				SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask), 0); //Add cause
				break;
				
			case kernel::Task::state::active:
				__BKPT(0);
				break;
				
			case kernel::Task::state::waiting:
				SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask), 1);    //TODO add cause
				break;
				
			case kernel::Task::state::ready:
				SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask), 2);   //TODO add cause
				break;
				
			default:
				break;
			}
#endif	
#ifdef SYSVIEW
				if (s_activeTask == &s_idle)
					SEGGER_SYSVIEW_OnIdle();
				else
					SEGGER_SYSVIEW_OnTaskStartExec(reinterpret_cast<uint32_t>(s_activeTask));
#endif
			s_previousTask = nullptr;  	//Context change done no need to know previous task anymore
		}
		
		
		//set pendSv, trigger context switch
		static void setPendSv()
		{
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
				if (s_ready.contain(readyTask))
					__BKPT(0);
				if (!s_ready.insert(readyTask, Task::priorityCompare))
					__BKPT(0);
				exitCriticalSection();
			}
			if (needSchedule)
				schedule();
			
		}
		
		static void hardFault()
		{
			__BKPT(0); //you fucked up
		}

		static void nmi()
		{
			__BKPT(0);	//you fucked up
		}

		static void usageFault()
		{
			__BKPT(0);	//you fucked up
		}

		static void busFault()
		{
			__BKPT(0);	//you fucked up
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
			
			askedService = (reinterpret_cast<ServiceCall::SvcNumber*>(args[6]))[-2]; //get stacked PC, then -2 to get SVC instruction 
			
			switch (askedService)
			{
			case ServiceCall::SvcNumber::startFirstTask:
				args[0] = startFirstTask();     //write result to stacked R0
				break;
				
			case ServiceCall::SvcNumber::registerIrq:
				args[0] = IrqRegister(static_cast<IRQn_Type>(param0), reinterpret_cast<IntVectManager::IrqHandler>(param1));
				break;
				
			case ServiceCall::SvcNumber::unregisterIrq:
				args[0] = IrqUnregister(static_cast<IRQn_Type>(param0));	//execute function and write result to stacked R0
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
				signalEvent(reinterpret_cast<Event*>(param0));
				break;
				
			case ServiceCall::SvcNumber::waitEvent:
				waitEvent(reinterpret_cast<Event*>(param0));
				break;
				
			case ServiceCall::SvcNumber::enableIrq:
				IntVectManager::enableIrq(static_cast<IRQn_Type>(param0));
				break;

			case ServiceCall::SvcNumber::disableIrq:
				IntVectManager::disableIrq(static_cast<IRQn_Type>(param0));
				break;

			case ServiceCall::SvcNumber::clearIrq:
				IntVectManager::clearIrq(static_cast<IRQn_Type>(param0));
				break;

			case ServiceCall::SvcNumber::setGlobalPriority:
				s_vectorTable.irqPriority(static_cast<IRQn_Type>(param0),static_cast<uint8_t>(param1));
				break;

			case ServiceCall::SvcNumber::setPriority:
				s_vectorTable.irqPriority(static_cast<IRQn_Type>(param0), static_cast<uint8_t>(param1), static_cast<uint8_t>(param2));
				break;
			case ServiceCall::SvcNumber::enterCriticalSection:
				IntVectManager::lockInterruptsLowerThan(s_systemPriority - 2);
				break;
				
			case ServiceCall::SvcNumber::exitCriticalSection:
				IntVectManager::lockInterruptsLowerThan(0);	//set basepri to 0 in order to enable all interrupts 
				break;

			default:	//unknown Service call number
				__BKPT(0);
				break;
			}
		}
		
		static void systickInit()
		{
			SysTick->LOAD = ((s_systemInterface->getSystemCoreFrequency() / 1000) - 1u);
			SysTick->VAL = 0;
			SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |  SysTick_CTRL_ENABLE_Msk;
		}
		
		static void idleTaskFunction(uint32_t param)
		{
			do
			{
#ifdef DEBUG
				__NOP();
#else
				__WFI();
#endif
			} while (true);
		}
#ifdef SYSVIEW
		static void sysviewInitInterruptStub()
		{
			for (uint16_t i = 0; i < s_sysviewVectorTable.getVectorTableSize(); i++)
			{
				s_sysviewVectorTable.registerHandler(i,&sysviewInterruptStub);
			}
			s_sysviewVectorTable.registerHandler(SVCall_IRQn, sysviewSvcStub);
			s_sysviewVectorTable.registerHandler(PendSV_IRQn, sysviewPendSvHandler);
		}
		
		static void sysviewInterruptStub() __attribute__((aligned(4)))
		{
			SEGGER_SYSVIEW_RecordEnterISR();			
			s_vectorTable.getIsr((SCB->ICSR & 0x3FF));
			SEGGER_SYSVIEW_RecordExitISR();
		}
		
		static void sysviewSvcStub()__attribute__((aligned(4))) __attribute__((naked))
		{
			uint32_t* stackedPointer;
			asm volatile(
				"TST LR,#4\n\t" //test bit 2 of EXC_RETURN to know if MSP or PSP 
				"ITE EQ\n\t"	//was used for stacking
				"MRSEQ R0, MSP\n\t"		//place msp or psp in R0 as parameter for SvcHandler
				"MRSNE R0, PSP\n\t" 
				"MOV %0,R0\n\t" : "=r" (stackedPointer) :);
			asm volatile("PUSH {LR}\n\t");     //stack LR to be able to return for exception
			SEGGER_SYSVIEW_RecordEnterISR();
			svcHandler(stackedPointer);
			SEGGER_SYSVIEW_RecordExitISR();
			asm volatile("POP {LR}");     //unstack LR 
			asm volatile("BX LR");    	//return from exception
		}
#endif
		
		
#ifdef SYSVIEW		
		//Handler of pendsv exception, act as context switcher
		static void __attribute__((naked)) __attribute__((aligned(4))) sysviewPendSvHandler()
		{
			uint32_t* stackPosition = nullptr;
			asm volatile(
				"MRS R0, PSP\n\t"	//store Process Stack pointer into R0
				"MOV R2,LR\n\t"		//store Link Register into R2
				"MRS R3,CONTROL\n\t"//store CONTROL register into R3
				"STMDB R0!,{R2-R11}\n\t" //store R4 to R11 memory pointed by R0 (stack), increment memory and rewrite the new adress to R0
				"MOV %0,R0\n\t" //store stack pointer in task.stackPointer
				"PUSH {LR}"	//save link return to be able to return later
				: "=r" (stackPosition));
			SEGGER_SYSVIEW_RecordExitISRToScheduler();
			enterKernelCriticalSection();
			changeTask(stackPosition);
			asm volatile(
				"MOV R0,%0\n\t"		//load stack pointer from task.stackPointer
				"LDMIA R0!,{R2-R11}\n\t"	//restore registers R4 to R11
				"MOV LR,R2\n\t"		//reload Link register
				"MSR CONTROL,R3\n\t"//reload CONTROL register
				"ISB\n\t"				//Instruction synchronisation Barrier is recommended after changing CONTROL
				"MSR PSP,R0\n\t"	//reload process stack pointer with task's stack pointer
				::"r" (s_activeTask->m_stackPointer));
			exitCriticalSection();
			asm volatile("POP { PC }");
		}		
#endif		
		
		static uint64_t getTicks()
		{
			return s_ticks;
		}
		
	};
	}
