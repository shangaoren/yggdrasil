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

#include "Scheduler.hpp"


namespace kernel
{
	Core* Scheduler::s_core;


	bool Scheduler::s_schedulerStarted = false;
	bool Scheduler::s_interruptInstalled = false;
	volatile uint64_t Scheduler::s_ticks = 0;
	volatile Scheduler::changeTaskTrigger Scheduler::s_trigger = Scheduler::changeTaskTrigger::none;
		
	Task* volatile Scheduler::s_activeTask = nullptr;
	Task* volatile Scheduler::s_taskToStack = nullptr;
	volatile bool Scheduler::scheduled = false;
	volatile uint8_t Scheduler::s_lockLevel = 0;
	uint32_t Scheduler::s_sysTickFreq = 1000;
	uint8_t Scheduler::s_systemPriority = 0;

	StartedList Scheduler::s_started;
	ReadyList Scheduler::s_ready;
	SleepingList Scheduler::s_sleeping;
	WaitableList Scheduler::s_waiting;
	TaskWithStack<256> Scheduler::s_idle = TaskWithStack<256>(idleTaskFunction, 0, 0,"Idle");
	
	
	/*-------------------------------------------------------------------------------------------*/
	/*                                                                                           */
	/*                                     PUBLIC FONCTION                                       */
	/*                                                                                           */
	/*-------------------------------------------------------------------------------------------*/

	void Scheduler::setupKernel(Core &systemCore, uint8_t systemPriority)
	{
		s_core = &systemCore;
		s_systemPriority = systemPriority;
		Y_ASSERT(s_systemPriority != 0); // Priority should be > 0
	}

	bool Scheduler::startKernel()
	{
		if (s_schedulerStarted) //Scheduler already started
			return false;
		if (s_ready.isEmpty()) //No Task in task list, cannot start Scheduler
			return false;
		s_idle.start(); //Add idle task
		if (!installKernelInterrupt())
			return false;
		//Init Systick
		s_core->systemTimer().initSystemTimer(s_core->coreClocks().getSystemCoreFrequency(), 1000);
		s_core->systemTimer().startSystemTimer();
		Hooks::onKernelStart();
		svc(ServiceCall::SvcNumber::startFirstTask); //Switch to service call to start the first task
		__BKPT(0);									 //shouldn't end here
		return true;
	}

	bool Scheduler::installKernelInterrupt()
	{
		if (s_interruptInstalled)
			return true;
		Y_ASSERT(s_core != nullptr);
		// Install vector manager if not already done
		if (!s_core->vectorManager().isInstalled())
		{
			if (!s_core->vectorManager().installVectorManager())
				return false;
		}
		//Setup Systick
		s_core->vectorManager().irqPriority(s_core->systemTimer().getIrq(), s_systemPriority);
		s_core->vectorManager().registerHandler(s_core->systemTimer().getIrq(), systickHandler, "I#15=Systick");

		//Setup Service Call interrupt
		s_core->vectorManager().irqPriority(SVCall_IRQn, s_systemPriority);
		s_core->vectorManager().registerHandler(SVCall_IRQn, asmSvcHandler, "I#11=ServiceCall");

		//Setup pendSV interrupt (used for task change)
		s_core->vectorManager().irqPriority(PendSV_IRQn, 0xFF); //Minimum priority for pendsv (task change)
		s_core->vectorManager().registerHandler(PendSV_IRQn, asmPendSv, "I#14=PendSV");
		s_interruptInstalled = true;
		return true;
	}

	bool __attribute__((aligned(4), optimize("O0"))) Scheduler::startFirstTask()
	{
		//start a task, reset main stack pointer
		s_activeTask = s_ready.getFirst();
		Hooks::onTaskStartExec(s_activeTask);
		s_schedulerStarted = true;
		asm volatile(
			"MOV R0,%0\n\t"			 //load stack pointer from task.stackPointer
			"LDMIA R0!,{R2-R11}\n\t" //restore registers R4 to R11
			"MOV LR,R2\n\t"			 //reload Link register
			"MSR CONTROL,R3\n\t"	 //reload CONTROL register
			"TST LR, #0x10\n\t"		 // test Bit 4 to know if FPU has to be restored, unstack if 0
			"IT EQ\n\t"
			"VLDMIAEQ R0!,{S16-S31}\n\t" //restore floating point registers
			"MSR PSP,R0\n\t"			 //reload process stack pointer with task's stack pointer
			"ISB\n\t"					 //Instruction synchronisation Barrier is recommended after changing CONTROL
			"BX LR" ::"r"(Scheduler::s_activeTask->m_stackPointer));

		return true; //should never return here
	}

	/*-------------------------------------------------------------------------------------------*/
	/*                                                                                           */
	/*                                    PRIVATE FONCTIONS                                      */
	/*                                                                                           */
	/*-------------------------------------------------------------------------------------------*/

	bool Scheduler::startTask(Task &task)
	{
		if (task.m_started)
			return false;
		s_started.insert(&task, Task::priorityCompare);
		s_ready.insert(&task, Task::priorityCompare);
		task.m_started = true;
		task.m_state = Task::state::ready;
		Hooks::onTaskStart(&task);
		if (s_schedulerStarted)
			schedule(kernel::Scheduler::changeTaskTrigger::taskStarted);
		return true;
	}

	bool Scheduler::schedule(changeTaskTrigger trigger)
	{
		scheduled = true;
		Y_ASSERT(s_ready.count() != 0); //assertion to check there is ready tasks
		if (s_activeTask != nullptr)
		{
			if (s_ready.peekFirst()->m_taskPriority > s_activeTask->m_taskPriority) //a task with higher priority is waiting, trigger context switching
			{
				//Store currently running task
				Y_ASSERT(!s_ready.contain(s_activeTask)); //currently running task not already in ready list
				Y_ASSERT(!s_sleeping.contain(s_activeTask));
				s_ready.insert(s_activeTask, Task::priorityCompare);
				Hooks::onTaskStopExec(s_activeTask);
				Hooks::onTaskReady(s_activeTask);
				if(s_taskToStack == nullptr)
					s_taskToStack = s_activeTask;
				s_activeTask = nullptr;
				//If there is no task to be saved already, put active task in s_previousTask to save context
				s_taskToStack->m_state = kernel::Task::state::ready;
				//Take the first available task
				s_activeTask = s_ready.getFirst();
				Y_ASSERT(s_activeTask != nullptr);
				setPendSv(trigger);
				return true;
			}
			else
			{
				return false;
			}
		}
		Y_ASSERT(false);
		return false;
	}

	bool Scheduler::stopTask(Task &task)
	{
		s_ready.remove(&task);
		s_sleeping.remove(&task);
		s_started.remove(&task);
		Hooks::onTaskClose(&task);
		if (s_activeTask == &task)
		{
			s_activeTask = s_ready.getFirst();
			asm volatile(
				"MOV R0,%0\n\t"			 //load stack pointer from task.stackPointer
				"LDMIA R0!,{R2-R11}\n\t" //restore registers R4 to R11
				"MOV LR,R2\n\t"			 //reload Link register
				"MSR CONTROL,R3\n\t"	 //reload CONTROL register
				"ISB\n\t"				 //Instruction synchronisation Barrier is recommended after changing CONTROL
				"MSR PSP,R0\n\t"		 //reload process stack pointer with task's stack pointer
				"BX LR" ::"r"(s_activeTask->m_stackPointer));
		}
		return true;
	}

	bool __attribute__((optimize("O0"))) Scheduler::sleep(uint32_t ms)
	{
		// should be triggered directly from task
		Y_ASSERT(s_activeTask != nullptr);
		Y_ASSERT(s_taskToStack == nullptr);
		s_taskToStack = s_activeTask;
		s_activeTask = nullptr; // no more active task
		s_taskToStack->m_wakeUpTimeStamp = s_ticks + ms;
		//Put active Task to sleep
		Y_ASSERT(!s_sleeping.contain(s_taskToStack)); // if active task already in sleeping list we have a problem
		s_sleeping.insert(s_taskToStack, Task::sleepCompare);
		s_taskToStack->m_state = Task::state::sleeping;
		Hooks::onTaskSleep(s_taskToStack, ms);
		s_activeTask = s_ready.getFirst();
		Y_ASSERT(s_activeTask != nullptr); // ready list should at least contain idle task
		setPendSv(kernel::Scheduler::changeTaskTrigger::enterSleep); //active task is sleeping, trigger context switch
		return true;
	}

	volatile uint32_t *__attribute__((optimize("O0"))) Scheduler::changeTask(uint32_t *stackPosition)
	{
		__DSB();
		if (s_trigger != kernel::Scheduler::changeTaskTrigger::none) // avoid Spurious interrupt
		{
			Y_ASSERT(s_activeTask != nullptr);
			Y_ASSERT(stackPosition > s_taskToStack->m_stackOrigin);
			Y_ASSERT(stackPosition < &s_taskToStack->m_stackOrigin[s_taskToStack->m_stackSize]);
			s_taskToStack->m_stackPointer = stackPosition; //save stackPosition
			s_taskToStack = nullptr;
			s_activeTask->m_state = kernel::Task::state::active;
			Hooks::onTaskStartExec(s_activeTask);
			s_trigger = kernel::Scheduler::changeTaskTrigger::none;
		}
		else
		{
			//Y_ASSERT(false); //trick to get breakpoint only when debug
			s_activeTask->m_stackPointer = stackPosition;
		}
		scheduled = false;
		return s_activeTask->m_stackPointer;
	}

	void Scheduler::checkStack()
	{
		uint32_t *stackPosition;
		asm volatile(
			"MRS R0, PSP\n\t" //store Process Stack pointer into R0
			"MOV %0,R0"		  //store stack pointer in task.stackPointer
			: "=r"(stackPosition));
		if (stackPosition == (nullptr))
			return;
		if (s_activeTask == nullptr)
			__BKPT(0);
		if ((stackPosition < (s_activeTask->m_stackOrigin)) || stackPosition > (&s_activeTask->m_stackOrigin[s_activeTask->m_stackSize - 1]))
		{
			__BKPT(0);
		}
	}

	void Scheduler::setPendSv(changeTaskTrigger trigger)
	{
		s_trigger = trigger;
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		__ISB();
	}

	void Scheduler::systickHandler()
	{
		bool needSchedule = false;
		s_ticks++;
		while (!s_sleeping.isEmpty() && (s_sleeping.peekFirst()->m_wakeUpTimeStamp) <= s_ticks) //one task or more is waiting, let's see if waiting is over
		{
			needSchedule = true;
			Task *readyTask = s_sleeping.getFirst();
			Y_ASSERT(readyTask != nullptr);
			readyTask->m_wakeUpTimeStamp = 0;
			readyTask->m_state = kernel::Task::state::ready;
			Y_ASSERT(!s_ready.contain(readyTask)); //new Ready task should not being already in ready list
			s_ready.insert(readyTask, Task::priorityCompare);
			Hooks::onTaskReady(readyTask);
		}
		while (!s_waiting.isEmpty() && (s_waiting.peekFirst()->m_wakeUpTimeStamp <= s_ticks))
		{
			Task *timeouted = s_waiting.getFirst();
			Y_ASSERT(timeouted->m_waitingFor != nullptr);
			timeouted->m_waitingFor->onTimeout(timeouted);
		}
		if (needSchedule)
			schedule(kernel::Scheduler::changeTaskTrigger::exitSleep);
	}

	void __attribute__((aligned(4))) Scheduler::svcHandler(uint32_t *args)
	{
		ServiceCall::SvcNumber askedService;
		uint32_t param0 = args[0], param1 = args[1], param2 = args[2];
		askedService = (reinterpret_cast<ServiceCall::SvcNumber *>(args[6]))[-2]; //get stacked PC, then -2 to get SVC instruction

		switch (askedService)
		{
		case ServiceCall::SvcNumber::startFirstTask:
			args[0] = startFirstTask(); //write result to stacked R0
			break;

		case ServiceCall::SvcNumber::registerIrq:
			args[0] = IrqRegister(static_cast<IVectorManager::Irq>(param0), reinterpret_cast<core::interfaces::IVectorManager::IrqHandler>(param1), reinterpret_cast<const char *>(param2));
			break;

		case ServiceCall::SvcNumber::unregisterIrq:
			args[0] = IrqUnregister(static_cast<IVectorManager::Irq>(param0)); //execute function and write result to stacked R0
			break;

		case ServiceCall::SvcNumber::startTask:
			args[0] = startTask(*(reinterpret_cast<Task *>(param0)));
			break;

		case ServiceCall::SvcNumber::stopTask:
			args[0] = stopTask(*(reinterpret_cast<Task *>(param0)));
			break;

		case ServiceCall::SvcNumber::sleepTask:
			args[0] = sleep(param0);
			break;

		case ServiceCall::SvcNumber::signalEvent:
			args[0] = Event::kernelSignalEvent(reinterpret_cast<Event *>(param0));
			break;

		case ServiceCall::SvcNumber::waitEvent:
			args[0] = Event::kernelWaitEvent(reinterpret_cast<Event *>(param0), param1);
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
			args[0] = s_core->vectorManager().lockInterruptsHigherThan(s_systemPriority - 2);
			break;

		case ServiceCall::SvcNumber::exitCriticalSection:
			s_core->vectorManager().unlockInterruptsHigherThan(args[0]); //set basepri to 0 in order to enable all interrupts
			break;

		case kernel::ServiceCall::SvcNumber::mutexLock:
			args[0] = Mutex::kernelLockMutex(reinterpret_cast<Mutex *>(param0), param1);
			break;
		case kernel::ServiceCall::SvcNumber::mutexRelease:
			args[0] = Mutex::kernelReleaseMutex(reinterpret_cast<Mutex *>(param0));
			break;

		default: //unknown Service call number
			__BKPT(0);
			break;
		}
	}

	uint64_t Scheduler::getTicks()
	{
		return s_ticks;
	}

	void Scheduler::idleTaskFunction(uint32_t param)
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

	extern "C" 
	{
	void __attribute__((naked, Optimize("O0"))) Scheduler::asmPendSv()
	{
		asm volatile(
			"CPSID I\n\t"
			"DSB\n\t"
			"MRS R0, PSP\n\t"				   //store Process Stack pointer into R0
			"TST LR, #0x10\n\t"				   // test Bit 4 of Link return to know if floating point was used , if 0 save FP
			"IT EQ\n\t"
			"VSTMDBEQ R0!, {S16-S31}\n\t" // save floating point registers
			"MOV R2,LR\n\t"				  //store Link Register into R2
			"MRS R3,CONTROL\n\t"		  //store CONTROL register into R3
			"STMDB R0!,{R2-R11}\n\t"	  //store R2 to R11 memory pointed by R0 (stack), increment memory and rewrite the new adress to R0
			"bl %[changeTask]\n\t"			// go to change task
			"LDMIA R0!,{R2-R11}\n\t"	  //restore registers R4 to R11 (R0 was loaded by change task with the new stackpointer
			"MOV LR,R2\n\t"				  //reload Link register
			"MSR CONTROL,R3\n\t"		  //reload CONTROL register
			"ISB\n\t"					  //Instruction synchronisation Barrier is recommended after changing CONTROL
			"TST LR, #0x10\n\t"			  // test Bit 4 to know if FPU has to be restored, unstack if 0
			"IT EQ\n\t"
			"VLDMIAEQ R0!,{S16-S31}\n\t" //restore floating point registers
			"MSR PSP,R0\n\t"			 //reload process stack pointer with task's stack pointer
			"CPSIE I\n\t" //unlock Interrupts
			"ISB \n\t"
			"BX LR"
				: // no outputs
			: [changeTask]"g"(Scheduler::changeTask)
				: "memory");
	}

	void __attribute__((naked, Optimize("O0"))) Scheduler::asmSvcHandler()
	{
		asm volatile(
			"TST LR,#4\n\t"		//test bit 2 of EXC_RETURN to know if MSP or PSP
			"ITE EQ\n\t"		//was used for stacking
			"MRSEQ R0, MSP\n\t" //place msp or psp in R0 as parameter for SvcHandler
			"MRSNE R0, PSP\n\t"
			"PUSH {LR}\n\t" //stack LR to be able to return for exception
			"bl %[svc]\n\t"
			"POP {LR}\n\t"
			"BX LR"
			: // no outputs
			: [svc] "g"(Scheduler::svcHandler):);
	}
	}

}	//End namespace kernel