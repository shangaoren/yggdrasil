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
#include "SEGGER_SYSVIEW.h"
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
		
		static bool startKernel(interface::ISystem& system, uint8_t systemInterruptPriority, uint8_t numberOfSubBits)
		{
			if (s_schedulerStarted)
				return false;
			if (s_ready.isEmpty())
				return false;

			s_systemInterface = &system;
			if(!s_systemInterface->initSystemClock())
				return false;

			installSystemInterrupts();
			systickInit();

			svc(ServiceCall::SvcNumber::startFirstTask);
			__BKPT(0);
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
		}

		static bool __attribute__((aligned(4))) startFirstTask()
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
		static interface::ISystem* s_systemInterface;
		static bool s_interruptInstalled;
		static uint8_t s_systemPriority;
		static uint8_t s_subPriorityBits;
		
		static ReadyList s_ready;
		static SleepingList s_sleeping;
		static StartedList s_started;
		
		
		static bool s_schedulerStarted;
		volatile static uint64_t s_ticks;
		
		static Task* s_activeTask;
		static uint32_t s_sysTickFreq;
		
		static TaskWithStack<30> s_idle;
		
		
		
		
		/****************************************************FUNCTIONS*************************************************/

		
		//register an Irq, only accessed via service call 
		static bool IrqRegister(IRQn_Type irq, IntVectManager::IrqHandler handler)
		{
			s_vectorTable.registerHandler(irq, handler);
			return true;
		}
		
		
		//unregister an Irq, only accessed via service call
		static bool IrqUnregister(IRQn_Type irq)
		{
			s_vectorTable.unregisterHandler(irq);
			return true;
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
			s_started.insert(&task, Task::priorityCompare);
			s_ready.insert(&task, Task::priorityCompare);
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
				s_ready.insert(s_activeTask, Task::priorityCompare);
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
				s_ready.insert(newReadyTask, Task::priorityCompare);
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
#ifdef SYSVIEW
				SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask), 0);  //TODO add cause
#endif
				event->m_list.insert(s_activeTask, Task::priorityCompare);
				setPendSv();
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
			s_ready.remove(&task);
			s_sleeping.remove(&task);
			s_started.remove(&task);
			return true;
		}
		
		
		//function to sleep a task for a number of ms
		static bool sleep(uint32_t ms)
		{
			s_activeTask->m_wakeUpTimeStamp = s_ticks + ms;	
#ifdef SYSVIEW
			SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask),0);
#endif
			s_activeTask->m_state = Task::state::sleeping;
			s_sleeping.insert(s_activeTask, Task::sleepCompare);
			setPendSv(); //active task is sleeping, trigger context switch
			return true;
		}
		
		
		
		
		
		//Handler of pendsv exception, act as context switcher
		static void __attribute__((naked)) __attribute__((aligned(4))) pendSvHandler()
		{
			uint32_t* stackPosition = nullptr;
			asm volatile(
				"MRS R0, PSP\n\t"	//store Process Stack pointer into R0
				"MOV R2,LR\n\t"		//store Link Register into R2
				"MRS R3,CONTROL\n\t"//store CONTROL register into R3
				"STMDB R0!,{R2-R11}\n\t" //store R4 to R11 memory pointed by R0 (stack), increment memory and rewrite the new adress to R0
				"MOV %0,R0\n\t" //store stack pointer in task.stackPointer
				: "=r" (stackPosition));
			//"MOV R1,%0\n\t"
			//"STR R0,[R1]\n\t" //store stack pointer in task.stackPointer
			//:: "r" (&s_activeTask->m_stackPointer) : "memory");
			//SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
			
			s_activeTask->m_stackPointer = stackPosition; //save stackPosition
			if(s_ready.isEmpty())
				s_activeTask = &s_idle;
			else
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
				Task* readyTask = s_sleeping.getFirst();
				readyTask->m_wakeUpTimeStamp = 0;
				s_ready.insert(readyTask, Task::priorityCompare);
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
		
		
		//Svc bootstrap, can't be in serviceCall.hpp :/
		static void __attribute__((aligned(4))) __attribute__((naked)) svcBootstrap()
		{
			uint32_t* stackedPointer;
			//uint32_t linkRegister;
			
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
			
			askedService = (reinterpret_cast<ServiceCall::SvcNumber*>(args[6]))[-2];
			
			switch (askedService)
			{
			case kernel::ServiceCall::SvcNumber::startFirstTask:
				args[0] = startFirstTask();     //write result to stacked R0
				break;
				
			case kernel::ServiceCall::SvcNumber::registerIrq:
				args[0] = IrqRegister(static_cast<IRQn_Type>(param0), reinterpret_cast<IntVectManager::IrqHandler>(param1));
				break;
				
			case kernel::ServiceCall::SvcNumber::unregisterIrq:
				args[0] = IrqUnregister(static_cast<IRQn_Type>(param0));	//execute function and write result to stacked R0
				break;
				
			case kernel::ServiceCall::SvcNumber::startTask:
				args[0] = startTask(*(reinterpret_cast<Task*>(param0)));
				break;
				
			case kernel::ServiceCall::SvcNumber::stopTask:
				args[0] = stopTask(*(reinterpret_cast<Task*>(param0)));
				break;
				
			case kernel::ServiceCall::SvcNumber::sleepTask:
				args[0] = sleep(param0);
				break;
				
			case kernel::ServiceCall::SvcNumber::signalEvent:
				signalEvent(reinterpret_cast<Event*>(param0));
				break;
				
			case kernel::ServiceCall::SvcNumber::waitEvent:
				waitEvent(reinterpret_cast<Event*>(param0));
				break;
				
			case kernel::ServiceCall::SvcNumber::enableIrq:
				IntVectManager::enableIrq(static_cast<IRQn_Type>(param0));
				break;

			case kernel::ServiceCall::SvcNumber::disableIrq:
				IntVectManager::disableIrq(static_cast<IRQn_Type>(param0));
				break;

			case kernel::ServiceCall::SvcNumber::clearIrq:
				IntVectManager::clearIrq(static_cast<IRQn_Type>(param0));
				break;

			case kernel::ServiceCall::SvcNumber::setGlobalPriority:
				s_vectorTable.irqPriority(static_cast<IRQn_Type>(param0),static_cast<uint8_t>(param1));
				break;

			case kernel::ServiceCall::SvcNumber::setPriority:
				s_vectorTable.irqPriority(static_cast<IRQn_Type>(param0), static_cast<uint8_t>(param1), static_cast<uint8_t>(param2));
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
			uint32_t linkRegister;
			
			asm volatile(
				"TST LR,#4\n\t" //test bit 2 of EXC_RETURN to know if MSP or PSP 
				"ITE EQ\n\t"	//was used for stacking
				"MRSEQ R0, MSP\n\t"		//place msp or psp in R0 as parameter for SvcHandler
				"MRSNE R0, PSP\n\t" 
				"MOV %0,R0\n\t" : "=r" (stackedPointer) :);
			asm volatile("PUSH {LR}\n\t");     //stack LR to be able to return for exception
			SEGGER_SYSVIEW_RecordEnterISR();
			SvcHandler(stackedPointer);
			SEGGER_SYSVIEW_RecordExitISR();
			asm volatile("POP {LR}");     //unstack LR 
			asm volatile("BX LR");    	//return from exception
		}
#endif
		
		static void changeTask(uint32_t* stackPosition)
		{
			s_activeTask->m_stackPointer = stackPosition;   //save stackPosition
			
#ifdef SYSVIEW
			SEGGER_SYSVIEW_OnTaskStopExec();
			switch (s_activeTask->m_state)
			{
			case kernel::Task::state::sleeping:
				SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask), 0);//Add cause
				break;
				
			case kernel::Task::state::active:
				__BKPT(0);
				break;
				
			case kernel::Task::state::waiting:
				SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask), 1);   //TODO add cause
				break;
				
			case kernel::Task::state::ready:
				SEGGER_SYSVIEW_OnTaskStopReady(reinterpret_cast<uint32_t>(s_activeTask), 2);  //TODO add cause
				break;
				
			default:
				break;
			}
#endif
			if (!s_ready.isEmpty())
			{
				s_activeTask = s_ready.getFirst();	
#ifdef SYSVIEW
				SEGGER_SYSVIEW_OnTaskStartExec(reinterpret_cast<uint32_t>(s_activeTask));
#endif
			}
			else
			{
				s_activeTask = &s_idle;
#ifdef SYSVIEW
				SEGGER_SYSVIEW_OnIdle();
#endif
			}

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
				: "=r" (stackPosition));

			SEGGER_SYSVIEW_RecordExitISRToScheduler();

			changeTask(stackPosition);		
		}		
#endif		
		/**Disable Interrupts, return true if success, false otherwise*/
		static bool lockInterrupts()
		{
			return true;
		}
		
		
		/*Enable Interrupts, return true if success, false otherwise*/
		static bool unlockInterrupt()
		{
			return true;
		}
		
		static uint64_t getTicks()
		{
			return s_ticks;
		}
		
	};
	}
