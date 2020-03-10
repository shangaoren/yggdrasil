#include "SystemView.hpp"
#include "yggdrasil/kernel/Scheduler.hpp"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/
#define ENABLE_STATE_OFF 0
#define ENABLE_STATE_ON 1
#define ENABLE_STATE_DROPPING 2

#define FORMAT_FLAG_LEFT_JUSTIFY (1u << 0)
#define FORMAT_FLAG_PAD_ZERO (1u << 1)
#define FORMAT_FLAG_PRINT_SIGN (1u << 2)
#define FORMAT_FLAG_ALTERNATE (1u << 3)

#define MODULE_EVENT_OFFSET (512)

namespace kernel
{

	uint8_t SystemView::lock(uint8_t lockLevel)
	{
		m_core->vectorManager().lockInterruptsHigherThan(2);
		return 0;
	}

	void SystemView::unlock(uint8_t lockLevel)
	{
		m_core->vectorManager().unlockInterruptsHigherThan(0x0);
	}

	void __attribute__((aligned(4))) __attribute__((naked, optimize("O0"))) SystemView::svcBootstrap()
	{
		uint32_t *stackedPointer;
		asm volatile(
			"TST LR,#4\n\t"		//test bit 2 of EXC_RETURN to know if MSP or PSP
			"ITE EQ\n\t"		//was used for stacking
			"MRSEQ R0, MSP\n\t" //place msp or psp in R0 as parameter for SvcHandler
			"MRSNE R0, PSP\n\t"
			"MOV %0,R0\n\t"
			: "=r"(stackedPointer)
			:);
		asm volatile("PUSH {LR}\n\t"); //stack LR to be able to return for exception
		s_self.recordEnterIsr();
		Scheduler::svcHandler(stackedPointer);
		s_self.recordExitIsr();
		asm volatile("POP {LR}\n\t"
					"BX LR"); //return from exception
	}

	void __attribute__((naked, optimize("O0"))) __attribute__((aligned(4))) SystemView::pendSvHandler()
	{
		uint32_t *stackPosition;
		asm(
			"CPSID I\n\t" //set primask (disable all interrupts)
			"ISB \n\t"
			"MRS R0, PSP\n\t" //store Process Stack pointer into R0
			"TST LR, #0x16\n\t" // test Bit 4 of Link return to know if floating point was used , if 0 save FP
			"IT EQ\n\t"
			"VSTMDBEQ R0!, {S16-S31}\n\t"	// save floating point registers
			"MOV R2,LR\n\t"				  //store Link Register into R2
			"MRS R3,CONTROL\n\t"		  //store CONTROL register into R3
			"STMDB R0!,{R2-R11}\n\t"	  //store R4 to R11 memory pointed by R0 (stack), increment memory and rewrite the new adress to R0
			"MOV %0,R0"					  //store stack pointer in task.stackPointer
			: "=r"(stackPosition));
		s_self.recordEnterIsr();
		Scheduler::changeTask(stackPosition);
		s_self.recordEnterIsr();
		asm volatile(
			"MOV R0,%0\n\t"			 //load stack pointer from task.stackPointer
			"LDMIA R0!,{R2-R11}\n\t" //restore registers R4 to R11
			"MOV LR,R2\n\t"			 //reload Link register
			"MSR CONTROL,R3\n\t"	 //reload CONTROL register
			"TST LR, #0x16\n\t"		 // test Bit 4 to know if FPU has to be restored, unstack if 0
			"IT EQ\n\t"
			"VLDMIAEQ R0!,{S16-S31}\n\t" //restore floating point registers
			"MSR PSP,R0\n\t"		 //reload process stack pointer with task's stack pointer
			"CPSIE I\n\t"			 //clear primask (enable interrupts)
			"ISB\n\t"				 //Instruction synchronisation Barrier is recommended after changing CONTROL
			"BX LR"
			::"r"(Scheduler::s_activeTask->m_stackPointer));
	}

	void SystemView::SysviewIsrHandler()
	{
		s_self.recordEnterIsr();
		Scheduler::s_core->vectorManager().getIsr(SCB->ICSR & 0x01FF)();
		s_self.recordExitIsr();
	}
	
	


	SystemView SystemView::s_self;

	/*********************************************************************
*
*       SEGGER_SYSVIEW_Start()
*
*  Function description
*    Start recording SystemView events.
*    This function is triggered by the host application.
*
*  Additional information
*    This function enables transmission of SystemView packets recorded
*    by subsequent trace calls and records a SystemView Start event.
*
*    As part of start, a SystemView Init packet is sent, containing the system
*    frequency. The list of current tasks, the current system time and the
*    system description string is sent, too.
*
*  Notes
*    SEGGER_SYSVIEW_Start and SEGGER_SYSVIEW_Stop do not nest.
*/
	void SystemView::start()
	{
		
		if (m_enableState == 0)
		{
			m_enableState = 1;
			m_lockLevel= lock(SEGGER_RTT_MAX_INTERRUPT_PRIORITY);
			SEGGER_RTT_WriteSkipNoLock(SEGGER_SYSVIEW_RTT_CHANNEL, reinterpret_cast<const void*>(Sync.data()), Sync.size());
			unlock(m_lockLevel);
			record(SYSVIEW_EVTID_TRACE_START);
			{
				uint8_t *pPayload;
				uint8_t *pPayloadStart;
				recordStart(pPayloadStart ,SEGGER_SYSVIEW_INFO_SIZE + 4 * SEGGER_SYSVIEW_QUANTA_U32);
				//
				pPayload = pPayloadStart;
				encodeU32(pPayload, m_core->coreClocks().getSystemCoreFrequency()); //encode system frequency (always 1kHz) 
				encodeU32(pPayload, m_core->coreClocks().getSystemCoreFrequency()); //encode cpu frequency
				encodeU32(pPayload, m_ramBaseAddress); // encode ram base Address
				encodeU32(pPayload, SEGGER_SYSVIEW_ID_SHIFT);
				sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_INIT);
				recordEnd();
				sendSystemDescription("N=" SYSVIEW_APP_NAME ",O=Yggdrasil,D=" SYSVIEW_DEVICE_NAME);
				recordSystemTime();
				sendTaskList();
				sendNumModules();
			}
		}
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_Stop()
*
*  Function description
*    Stop recording SystemView events.
*
*  Additional information
*    This function disables transmission of SystemView packets recorded
*    by subsequent trace calls.  If transmission is enabled when
*    this function is called, a single SystemView Stop event is recorded
*    to the trace, send, and then trace transmission is halted.
*/
	void SystemView::stop()
	{
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart ,SEGGER_SYSVIEW_INFO_SIZE);
		if (m_enableState)
		{
			sendPacket(pPayloadStart, pPayloadStart, SYSVIEW_EVTID_TRACE_STOP);
			m_enableState = 0;
		}
		recordEnd();
	}

	void SystemView::init(kernel::Core &systemCore)
	{
		if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0)
		{
			CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
		}	
		m_core = &systemCore;
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; //activate cycle counter
		SEGGER_RTT_ConfigUpBuffer(SEGGER_SYSVIEW_RTT_CHANNEL, "SysView", &m_upBuffer[0], m_upBuffer.size(), SEGGER_RTT_MODE_NO_BLOCK_SKIP);
		SEGGER_RTT_ConfigDownBuffer(SEGGER_SYSVIEW_RTT_CHANNEL, "SysView", &m_downBuffer[0], m_downBuffer.size(), SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_GetSysDesc()
*
*  Function description
*    Triggers a send of the system information and description.
*
*/
	void SystemView::systemDescription()
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart ,SEGGER_SYSVIEW_INFO_SIZE + 4 * SEGGER_SYSVIEW_QUANTA_U32);
		//
		pPayload = pPayloadStart;
		encodeU32(pPayload, m_core->coreClocks().getSystemCoreFrequency());
		encodeU32(pPayload, m_core->coreClocks().getSystemCoreFrequency());
		encodeU32(pPayload, m_ramBaseAddress);
		encodeU32(pPayload, SEGGER_SYSVIEW_ID_SHIFT);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_INIT);
		recordEnd();
		sendSystemDescription("N=" SYSVIEW_APP_NAME ",O=Yggdrasil,D=" SYSVIEW_DEVICE_NAME);
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_SendTaskInfo()
* 
*  Function description
*    Send a Task Info Packet, containing TaskId for identification,
*    task priority and task name.
*
*  Parameters
*    pInfo - Pointer to task information to send.
*/
	void SystemView::sendTaskInfo(const kernel::Task* task)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart,SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32 + 1 + 32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, shrinkId(reinterpret_cast<uint32_t>(task->m_stackOrigin)));
		encodeU32(pPayload, task->m_taskPriority);
		pPayload = encodeString(pPayload, task->m_name, 32);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_TASK_INFO);
		pPayload = pPayloadStart;
		encodeU32(pPayload, shrinkId(reinterpret_cast<uint32_t>(task->m_stackOrigin)));
		encodeU32(pPayload, reinterpret_cast<uint32_t>(task->m_stackOrigin));
		encodeU32(pPayload, task->m_stackSize);
		encodeU32(pPayload, 0); // Stack End, future use
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_STACK_INFO);
		recordEnd();
	}

	void SystemView::sendTaskList()
	{
		m_lockLevel = lock(SEGGER_RTT_MAX_INTERRUPT_PRIORITY);
		if (kernel::Scheduler::s_started.count() != 0)
		{
			for (uint32_t i = 0; i < kernel::Scheduler::s_started.count(); i++)
			{
				sendTaskInfo(kernel::Scheduler::s_started[i]);
			}
		}
		unlock(m_lockLevel);
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_SendSysDesc()
*
*  Function description
*    Send the system description string to the host.
*    The system description is used by SystemViewer to identify the
*    current application and handle events accordingly.
*
*  Parameters
*    sSysDesc - Pointer to the 0-terminated system description string.
*
*  Additional information
*    One system description string may not exceed SEGGER_SYSVIEW_MAX_STRING_LEN characters.
*
*    The Following items can be described in a system description string.
*    Each item is identified by its identifier, followed by '=' and the value.
*    Items are separated by ','.
*/
	void SystemView::sendSystemDescription(const char *systemDesc)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 1 + SEGGER_SYSVIEW_MAX_STRING_LEN);
		pPayload = encodeString(pPayloadStart, systemDesc, SEGGER_SYSVIEW_MAX_STRING_LEN);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_SYSDESC);
		recordEnd();
	}
	
	void SystemView::registerInterrupt(const char*name)
	{
		if(name != nullptr)
			sendSystemDescription(name);
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_IsStarted()
*
*  Function description
*    Handle incoming packets if any and check if recording is started.
*
*  Return value
*      0: Recording not started.
*    > 0: Recording started.
*/
	int16_t SystemView::isStarted()
	{
		if(rttHasData(SEGGER_SYSVIEW_RTT_CHANNEL))
		{
			if (m_recurtionCounter == 0)
			{ // Avoid uncontrolled nesting. This way, this routine can call itself once, but no more often than that.
				m_recurtionCounter = 1;
				handleIncomingPacket();
				m_recurtionCounter = 0;
			}
		}
		return m_enableState;
	}

	/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordVoid()
*
*  Function description
*    Formats and sends a SystemView packet with an empty payload.
*
*  Parameters
*    EventID - SystemView event ID.
*/
	void SystemView::record(unsigned int EventId)
	{
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE);
		sendPacket(pPayloadStart, pPayloadStart, EventId);
		recordEnd();
	}

	/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordString()
*
*  Function description
*    Formats and sends a SystemView packet containing a string.
*
*  Parameters
*    EventID - SystemView event ID.
*    pString - The string to be sent in the SystemView packet payload.
*
*  Additional information
*    The string is encoded as a count byte followed by the contents
*    of the string.
*    No more than SEGGER_SYSVIEW_MAX_STRING_LEN bytes will be encoded to the payload.
*/
	void SystemView::record(unsigned int eventId, const char *string)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 1 + SEGGER_SYSVIEW_MAX_STRING_LEN);
		pPayload = encodeString(pPayloadStart, string, SEGGER_SYSVIEW_MAX_STRING_LEN);
		sendPacket(pPayloadStart, pPayload, eventId);
		recordEnd();
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordSystime()
*
*  Function description
*    Formats and sends a SystemView Systime containing a single U64 or U32
*    parameter payload.
*/
	void SystemView::recordSystemTime()
	{
		uint64_t Systime;
		Systime = *(uint32_t *)(0xE0001004);
		record(SYSVIEW_EVTID_SYSTIME_US,static_cast<uint32_t>(Systime));
		//record(SYSVIEW_EVTID_SYSTIME_CYCLES, SEGGER_SYSVIEW_GET_TIMESTAMP());		
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordEnterISR()
*
*  Function description
*    Format and send an ISR entry event.
*
*  Additional information
*    Example packets sent
*      02 0F 50              // ISR(15) Enter. Timestamp is 80 (0x50)
*/
	void SystemView::recordEnterIsr()
	{
		unsigned v;
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		v = SEGGER_SYSVIEW_GET_INTERRUPT_ID();
		encodeU32(pPayload, v);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_ISR_ENTER);
		recordEnd();
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordExitISR()
*
*  Function description
*    Format and send an ISR exit event.
*
*  Additional information
*    Format as follows:
*      03 <TimeStamp>        // Max. packet len is 6
*
*    Example packets sent
*      03 20                // ISR Exit. Timestamp is 32 (0x20)
*/
	void SystemView::recordExitIsr()
	{
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE);
		sendPacket(pPayloadStart, pPayloadStart, SYSVIEW_EVTID_ISR_EXIT);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordExitISRToScheduler()
*
*  Function description
*    Format and send an ISR exit into scheduler event.
*
*  Additional information
*    Format as follows:
*      18 <TimeStamp>        // Max. packet len is 6
*
*    Example packets sent
*      18 20                // ISR Exit to Scheduler. Timestamp is 32 (0x20)
*/
	void SystemView::recordIsrToScheduler()
	{
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE);
		sendPacket(pPayloadStart, pPayloadStart, SYSVIEW_EVTID_ISR_TO_SCHEDULER);
		recordEnd();
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordEnterTimer()
*
*  Function description
*    Format and send a Timer entry event.
*  
*  Parameters
*    TimerId - Id of the timer which starts.
*/
	void SystemView::recordEnterTimer(uint32_t timerId)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, shrinkId(timerId));
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_TIMER_ENTER);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordExitTimer()
*
*  Function description
*    Format and send a Timer exit event.
*/
	void SystemView::recordExitTimer()
	{
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE);
		sendPacket(pPayloadStart, pPayloadStart, SYSVIEW_EVTID_TIMER_EXIT);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordEndCall()
*
*  Function description
*    Format and send an End API Call event without return value.
*  
*  Parameters
*    EventID - Id of API function which ends.
*/
	void SystemView::recordEndCall(unsigned int eventId)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, eventId);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_END_CALL);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordEndCall()
*
*  Function description
*    Format and send an End API Call event with return value.
*  
*  Parameters
*    EventID      - Id of API function which ends.
*    Para0        - Return value which will be returned by the API function.
*/
	void SystemView::recordEndCall(unsigned int eventId, uint32_t param)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 2 * SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, eventId,param);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_END_CALL);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_OnIdle()
*
*  Function description
*    Record an Idle event.
*/
	void SystemView::onIdle()
	{
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE);
		sendPacket(pPayloadStart, pPayloadStart, SYSVIEW_EVTID_IDLE);
		recordEnd();
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_OnTaskCreate()
*
*  Function description
*    Record a Task Create event.  The Task Create event corresponds
*    to creating a task in the OS.
*
*  Parameters
*    TaskId        - Task ID of created task.
*/
	void SystemView::onTaskCreate(const kernel::Task* task)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, shrinkId(reinterpret_cast<uint32_t>(task->m_stackOrigin)));
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_TASK_CREATE);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_OnTaskTerminate()
*
*  Function description
*    Record a Task termination event.  
*    The Task termination event corresponds to terminating a task in 
*    the OS. If the TaskId is the currently active task, 
*    SEGGER_SYSVIEW_OnTaskStopExec may be used, either.
*
*  Parameters
*    TaskId        - Task ID of terminated task.
*/
	void SystemView::onTaskTerminate(const kernel::Task *task)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, shrinkId(reinterpret_cast<uint32_t>(task->m_stackOrigin)));
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_TASK_TERMINATE);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_OnTaskStartExec()
*
*  Function description
*    Record a Task Start Execution event.  The Task Start event
*    corresponds to when a task has started to execute rather than
*    when it is ready to execute.
*
*  Parameters
*    TaskId - Task ID of task that started to execute.
*/
	void SystemView::onTaskStartExec(const kernel::Task *task)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, shrinkId(reinterpret_cast<uint32_t>(task->m_stackOrigin)));
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_TASK_START_EXEC);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_OnTaskStopExec()
*
*  Function description
*    Record a Task Stop Execution event.  The Task Stop event
*    corresponds to when a task stops executing and terminates.
*/
	void SystemView::onTaskStopExec()
	{
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE);
		sendPacket(pPayloadStart, pPayloadStart, SYSVIEW_EVTID_TASK_STOP_EXEC);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_OnTaskStartReady()
*
*  Function description
*    Record a Task Start Ready event.
*
*  Parameters
*    TaskId - Task ID of task that started to execute.
*/
	void SystemView::onTaskStartReady(const kernel::Task *task)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, shrinkId(reinterpret_cast<uint32_t>(task->m_stackOrigin)));
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_TASK_START_READY);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_OnTaskStopReady()
*
*  Function description
*    Record a Task Stop Ready event.
*
*  Parameters
*    TaskId - Task ID of task that completed execution.
*    Cause  - Reason for task to stop (i.e. Idle/Sleep)
*/
	void SystemView::onTaskStopReady(const kernel::Task *task, unsigned int cause)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 2 * SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, shrinkId(reinterpret_cast<uint32_t>(task->m_stackOrigin)), cause);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_TASK_STOP_READY);
		recordEnd();
	}
/*********************************************************************
*
*       SEGGER_SYSVIEW_OnUserStart()
*
*  Function description
*    Send a user event start, such as start of a subroutine for profiling.
*
*  Parameters
*    UserId  - User defined ID for the event.
*/
	void SystemView::onUserStart(unsigned int Id)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, Id);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_USER_START);
		recordEnd();
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_OnUserStop()
*
*  Function description
*    Send a user event stop, such as return of a subroutine for profiling.
*
*  Parameters
*    UserId  - User defined ID for the event.
*/
	void SystemView::onUserStop(unsigned int userId)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, userId);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_USER_STOP);
		recordEnd();
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_NameResource()
*
*  Function description
*    Send the name of a resource to be displayed in SystemViewer.
*
*  Parameters
*    ResourceId - Id of the resource to be named. i.e. its address.
*    sName      - Pointer to the resource name. (Max. SEGGER_SYSVIEW_MAX_STRING_LEN Bytes)
*/
	void SystemView::nameRessource(uint32_t resourceId, const char *name)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_QUANTA_U32 + 1 + SEGGER_SYSVIEW_MAX_STRING_LEN);
		pPayload = pPayloadStart;
		encodeU32(pPayload, shrinkId(resourceId));
		pPayload = encodeString(pPayload, name, SEGGER_SYSVIEW_MAX_STRING_LEN);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_NAME_RESOURCE);
		recordEnd();
	}

	

	/*********************************************************************
*
*       SEGGER_SYSVIEW_SendPacket()
*
*  Function description
*    Send an event packet.
*
*  Parameters
*    pPacket      - Pointer to the start of the packet.
*    pPayloadEnd  - Pointer to the end of the payload.
*                   Make sure there are at least 5 bytes free after the payload.
*    EventId      - Id of the event packet.
*
*  Return value
*    !=0:  Success, Message sent.
*    ==0:  Buffer full, Message *NOT* sent.
*/
	uint16_t SystemView::sendPacketWithLock(uint8_t *packet, uint8_t *packetEnd, unsigned int eventId)
	{

		m_lockLevel = lock(SEGGER_RTT_MAX_INTERRUPT_PRIORITY);
		sendPacket(packet + 4, packetEnd, eventId);
		unlock(m_lockLevel);
		return 0;
	}
	
	
/*********************************************************************
*
*       SEGGER_SYSVIEW_RegisterModule()
*
*  Function description
*    Register a middleware module for recording its events.
*
*  Parameters
*    pModule  - The middleware module information.
*
*  Additional information
*    SEGGER_SYSVIEW_MODULE elements:
*      sDescription      - Pointer to a string containing the module name and optionally the module event description.
*      NumEvents         - Number of events the module wants to register.
*      EventOffset       - Offset to be added to the event Ids. Out parameter, set by this function. Do not modify after calling this function.
*      pfSendModuleDesc  - Callback function pointer to send more detailed module description to SystemViewer.
*      pNext             - Pointer to next registered module. Out parameter, set by this function. Do not modify after calling this function.
*/
	void SystemView::registerModule(SysviewModule &module)
	{
		m_lockLevel = lock(SEGGER_RTT_MAX_INTERRUPT_PRIORITY);
		if (m_first == nullptr)
		{
			//
			// No module registered, yet.
			// Start list with new module.
			// EventOffset is the base offset for modules
			//

			module.m_eventOffset = MODULE_EVENT_OFFSET;
			module.m_next = nullptr;
			m_first = &module;
			m_moduleCount = 1;
		}
		else
		{
			//
			// Registreded module(s) present.
			// Prepend new module in list.
			// EventOffset set from number of events and offset of previous module.
			//
			module.m_eventOffset = m_first->m_eventOffset + m_first->m_numEvents;
			module.m_next = m_first;
			m_first = &module;
			m_moduleCount++;
		}
		sendModule(0);
		sendModuleDescription();
		unlock(m_lockLevel);
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordModuleDescription()
*
*  Function description
*    Sends detailed information of a registered module to the host.
*
*  Parameters
*    pModule      - Pointer to the described module.
*    sDescription - Pointer to a description string.
*/
	void SystemView::recordModuleDescription(const SysviewModule &module, const char *description)
	{
		uint8_t ModuleId;
		SysviewModule *p;

		p = m_first;
		ModuleId = 0;
		do
		{
			if (p == &module)
			{
				break;
			}
			ModuleId++;
			p = p->m_next;
		} while (p);
		{
			uint8_t *pPayload;
			uint8_t *pPayloadStart;
			recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 2 * SEGGER_SYSVIEW_QUANTA_U32 + 1 + SEGGER_SYSVIEW_MAX_STRING_LEN);
			pPayload = pPayloadStart;
			//
			// Send module description
			// Send event offset and number of events
			//
			encodeU32(pPayload, ModuleId, (module.m_eventOffset));
			pPayload = encodeString(pPayload, description, SEGGER_SYSVIEW_MAX_STRING_LEN);
			sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_MODULEDESC);
			recordEnd();
		}
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_SendModule()
*
*  Function description
*    Sends the information of a registered module to the host.
*
*  Parameters
*    ModuleId   - Id of the requested module.
*/
	void SystemView::sendModule(uint8_t moduleId)
	{
		SysviewModule *pModule;
		uint32_t n;

		if (m_first != nullptr)
		{
			pModule = m_first;
			for (n = 0; n < moduleId; n++)
			{
				pModule = pModule->m_next;
				if (pModule == 0)
				{
					break;
				}
			}
			if (pModule != nullptr)
			{
				uint8_t *pPayload;
				uint8_t *pPayloadStart;
				recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 2 * SEGGER_SYSVIEW_QUANTA_U32 + 1 + SEGGER_SYSVIEW_MAX_STRING_LEN);
				pPayload = pPayloadStart;
				//
				// Send module description
				// Send event offset and number of events
				//
				encodeU32(pPayload, moduleId, (pModule->m_eventOffset));
				pPayload = encodeString(pPayload, pModule->m_module, SEGGER_SYSVIEW_MAX_STRING_LEN);
				sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_MODULEDESC);
				recordEnd();
			}
		}
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_SendModuleDescription()
*
*  Function description
*    Triggers a send of the registered module descriptions.
*
*/
	void SystemView::sendModuleDescription()
	{
		SysviewModule *pModule;

		if (m_first != nullptr)
		{
			pModule = m_first;
			do
			{
				if (pModule->m_sendModuleDescription != nullptr)
				{
					pModule->m_sendModuleDescription();
				}
				pModule = pModule->m_next;
			} while (pModule != nullptr);
		}
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_SendNumModules()
*
*  Function description
*    Send the number of registered modules to the host.
*/
	void SystemView::sendNumModules()
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 2 * SEGGER_SYSVIEW_QUANTA_U32);
		pPayload = pPayloadStart;
		encodeU32(pPayload, m_moduleCount);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_NUMMODULES);
		recordEnd();
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_PrintfHostEx()
*
*  Function description
*    Print a string which is formatted on the host by SystemViewer
*    with Additional information.
*
*  Parameters
*    s        - String to be formatted.
*    Options  - Options for the string. i.e. Log level.
*
*  Additional information
*    All format arguments are treated as 32-bit scalar values.
*/
	void SystemView::printfHostEx(const char *s, uint32_t options, ...)
	{
		va_list ParamList;
		va_start(ParamList, options);
		vPrintHost(s, options, &ParamList);
		va_end(ParamList);
	}
	/*********************************************************************
*
*       SEGGER_SYSVIEW_PrintfHost()
*
*  Function description
*    Print a string which is formatted on the host by SystemViewer.
*
*  Parameters
*    s        - String to be formatted.
*
*  Additional information
*    All format arguments are treated as 32-bit scalar values.
*/
	void SystemView::printfHost(const char *s, ...)
	{
		va_list ParamList;
		va_start(ParamList, s);
		vPrintHost(s, SEGGER_SYSVIEW_LOG, &ParamList);
		va_end(ParamList);
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_WarnfHost()
*
*  Function description
*    Print a warnin string which is formatted on the host by 
*    SystemViewer.
*
*  Parameters
*    s        - String to be formatted.
*
*  Additional information
*    All format arguments are treated as 32-bit scalar values.
*/
	void SystemView::warnfHost(const char *s, ...)
	{
		va_list ParamList;
		va_start(ParamList, s);
		vPrintHost(s, SEGGER_SYSVIEW_WARNING, &ParamList);
		va_end(ParamList);
	}

	/*********************************************************************
*
*       SEGGER_SYSVIEW_ErrorfHost()
*
*  Function description
*    Print an error string which is formatted on the host by 
*    SystemViewer.
*
*  Parameters
*    s        - String to be formatted.
*
*  Additional information
*    All format arguments are treated as 32-bit scalar values.
*/
	void SystemView::errorfHost(const char *s, ...)
	{
		va_list ParamList;
		va_start(ParamList, s);
		vPrintHost(s, SEGGER_SYSVIEW_ERROR, &ParamList);
		va_end(ParamList);
	}

	/*********************************************************************
*
*       SEGGER_SYSVIEW_PrintfTargetEx()
*
*  Function description
*    Print a string which is formatted on the target before sent to 
*    the host with Additional information.
*
*  Parameters
*    s        - String to be formatted.
*    Options  - Options for the string. i.e. Log level.
*/
	void SystemView::printfTargetEx(const char *s, uint32_t options, ...)
	{
		va_list ParamList;

		va_start(ParamList, options);
		vPrintTarget(s, options, &ParamList);
		va_end(ParamList);
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_PrintfTarget()
*
*  Function description
*    Print a string which is formatted on the target before sent to 
*    the host.
*
*  Parameters
*    s        - String to be formatted.
*/
	void SystemView::printfTarget(const char *s, ...)
	{
		va_list ParamList;
		va_start(ParamList, s);
		vPrintTarget(s, SEGGER_SYSVIEW_LOG, &ParamList);
		va_end(ParamList);
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_WarnfTarget()
*
*  Function description
*    Print a warning string which is formatted on the target before
*    sent to the host.
*
*  Parameters
*    s        - String to be formatted.
*/
	void SystemView::warnfTarget(const char *s, ...)
	{
		va_list ParamList;
		va_start(ParamList, s);
		vPrintTarget(s, SEGGER_SYSVIEW_WARNING, &ParamList);
		va_end(ParamList);
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_ErrorfTarget()
*
*  Function description
*    Print an error string which is formatted on the target before
*    sent to the host.
*
*  Parameters
*    s        - String to be formatted.
*/
	
	void SystemView::errorfTarget(const char *s, ...)
	{
		va_list ParamList;
		va_start(ParamList, s);
		vPrintTarget(s, SEGGER_SYSVIEW_ERROR, &ParamList);
		va_end(ParamList);
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_Print()
*
*  Function description
*    Print a string to the host.
*
*  Parameters
*    s        - String to sent.
*/
	void SystemView::print(const char *s)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 2 * SEGGER_SYSVIEW_QUANTA_U32 + SEGGER_SYSVIEW_MAX_STRING_LEN);
		pPayload = encodeString(pPayloadStart, s, SEGGER_SYSVIEW_MAX_STRING_LEN);
		encodeU32(pPayload, SEGGER_SYSVIEW_LOG, 0);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_PRINT_FORMATTED);
		recordEnd();
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_Warn()
*
*  Function description
*    Print a warning string to the host.
*
*  Parameters
*    s        - String to sent.
*/
	void SystemView::warn(const char *s)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 2 * SEGGER_SYSVIEW_QUANTA_U32 + SEGGER_SYSVIEW_MAX_STRING_LEN);
		pPayload = encodeString(pPayloadStart, s, SEGGER_SYSVIEW_MAX_STRING_LEN);
		encodeU32(pPayload, SEGGER_SYSVIEW_WARNING, 0);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_PRINT_FORMATTED);
		recordEnd();
	}

/*********************************************************************
*
*       SEGGER_SYSVIEW_Error()
*
*  Function description
*    Print an error string to the host.
*
*  Parameters
*    s        - String to sent.
*/
	void SystemView::error(const char *s)
	{
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + 2 * SEGGER_SYSVIEW_QUANTA_U32 + SEGGER_SYSVIEW_MAX_STRING_LEN);
		pPayload = encodeString(pPayloadStart, s, SEGGER_SYSVIEW_MAX_STRING_LEN);
		encodeU32(pPayload, SEGGER_SYSVIEW_ERROR, 0);
		sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_PRINT_FORMATTED);
		recordEnd();
	}

	/*********************************************************************
*
*       SEGGER_SYSVIEW_EnableEvents()
*
*  Function description
*    Enable standard SystemView events to be generated.
*
*  Parameters
*    EnableMask   - Events to be enabled.
*/
	
	void SystemView::enableEvents(uint32_t enableMask)
	{
		m_disabledEvents &= enableMask;
	}

	/*********************************************************************
*
*       SEGGER_SYSVIEW_DisableEvents()
*
*  Function description
*    Disable standard SystemView events to not be generated.
*
*  Parameters
*    DisableMask  - Events to be disabled.
*/
	
	void SystemView::disableEvents(uint32_t disableMask)
	{
		m_disabledEvents |= disableMask;
	}

	/* ------------------------------Private Functions --------------------------*/

	/*********************************************************************
*
*       SEGGER_SYSVIEW_EncodeU32()
*
*  Function description
*    Encode a U32 in variable-length format.
*
*  Parameters
*    pPayload - Pointer to where U32 will be encoded.
*    Value    - The 32-bit value to be encoded.
*
*  Return value
*    Pointer to the byte following the value, i.e. the first free
*    byte in the payload and the next position to store payload
*    content.
*/
	uint8_t *SystemView::encode(uint8_t *payload, uint32_t value)
	{
		encodeU32(payload, value);
		return payload;
	}
	uint8_t *SystemView::encode(uint8_t *payload, const char *src, unsigned int len)
	{
		unsigned int n;
		n = 0;
		*payload++ = len;
		while (n < len)
		{
			*payload++ = *src++;
			n++;
		}
		return payload;
	}

	/*********************************************************************
*
*       _EncodeStr()
*
*  Function description
*    Encode a string in variable-length format.
*
*  Parameters
*    pPayload - Pointer to where string will be encoded.
*    pText    - String to encode.
*    Limit    - Maximum number of characters to encode from string.
*
*  Return value
*    Pointer to the byte following the value, i.e. the first free
*    byte in the payload and the next position to store payload
*    content.
*
*  Additional information
*    The string is encoded as a count byte followed by the contents
*    of the string.
*    No more than 1 + Limit bytes will be encoded to the payload.
*/
	uint8_t *SystemView::encodeString(uint8_t *payload, const char *string, unsigned int maxLen)
	{
		unsigned int n;
		unsigned int Len;
		//
		// Compute string len
		//
		Len = 0;
		while (*(string + Len) != 0)
		{
			Len++;
		}
		if (Len > maxLen)
		{
			Len = maxLen;
		}
		//
		// Write Len
		//
		if (Len < 255)
		{
			*payload++ = Len;
		}
		else
		{
			*payload++ = 255;
			*payload++ = (Len & 255);
			*payload++ = ((Len >> 8) & 255);
		}
		//
		// copy string
		//
		n = 0;
		while (n < Len)
		{
			*payload++ = *string++;
			n++;
		}
		return payload;
	}
	uint8_t *SystemView::encodeId(uint8_t *payload, uint32_t id)
	{
		id = shrinkId(id);
		encodeU32(payload, id);
		return payload;
	}
	uint32_t SystemView::shrinkId(uint32_t id)
	{
		return (((id)-m_ramBaseAddress) >> SEGGER_SYSVIEW_ID_SHIFT);
	}

	/*********************************************************************
*
*       _PreparePacket()
*
*  Function description
*    Prepare a SystemView event packet header.
*
*  Parameters
*    pPacket - Pointer to start of packet to initialize.
*
*  Return value
*    Pointer to first byte of packet payload.
*
*  Additional information
*    The payload length and evnetId are not initialized.
*    PreparePacket only reserves space for them and they are
*    computed and filled in by the sending function.
*/
	uint8_t *SystemView::preparePacket(uint8_t *packet)
	{
		return packet + 4;
	}

/*********************************************************************
*
*       _HandleIncomingPacket()
*
*  Function description
*    Read an incoming command from the down channel and process it.
*
*  Additional information
*    This function is called each time after sending a packet.
*    Processing incoming packets is done asynchronous. SystemView might
*    already have sent event packets after the host has sent a command.
*/
	
	void SystemView::handleIncomingPacket()
	{
		uint8_t Cmd;
		int Status;
		//
		Status = SEGGER_RTT_ReadNoLock(SEGGER_SYSVIEW_RTT_CHANNEL, &Cmd, 1);
		if (Status > 0)
		{
			switch (Cmd)
			{
			case SEGGER_SYSVIEW_COMMAND_ID_START:
				start();
				break;
			case SEGGER_SYSVIEW_COMMAND_ID_STOP:
				stop();
				break;
			case SEGGER_SYSVIEW_COMMAND_ID_GET_SYSTIME:
				recordSystemTime();
				break;
			case SEGGER_SYSVIEW_COMMAND_ID_GET_TASKLIST:
				sendTaskList();
				break;
			case SEGGER_SYSVIEW_COMMAND_ID_GET_SYSDESC:
				systemDescription();
				break;
			case SEGGER_SYSVIEW_COMMAND_ID_GET_NUMMODULES:
				sendNumModules();
				break;
			case SEGGER_SYSVIEW_COMMAND_ID_GET_MODULEDESC:
				sendModuleDescription();
				break;
			case SEGGER_SYSVIEW_COMMAND_ID_GET_MODULE:
				Status = SEGGER_RTT_ReadNoLock(SEGGER_SYSVIEW_RTT_CHANNEL, &Cmd, 1);
				if (Status > 0)
				{
					sendModule(Cmd);
				}
				break;
			default:
				if (Cmd >= 128)
				{ // Unknown extended command. Dummy read its parameter.
					SEGGER_RTT_ReadNoLock(SEGGER_SYSVIEW_RTT_CHANNEL, &Cmd, 1);
				}
				break;
			}
		}
	}

	/*********************************************************************
*
*       _TrySendOverflowPacket()
*
*  Function description
*    Try to transmit an SystemView Overflow packet containing the
*    number of dropped packets.
*
*  Additional information
*    Format as follows:
*      01 <DropCnt><TimeStamp>  Max. packet len is 1 + 5 + 5 = 11
*
*    Example packets sent
*      01 20 40
*
*  Return value
*    !=0:  Success, Message sent (stored in RTT-Buffer)
*    ==0:  Buffer full, Message *NOT* stored
*
*/
	int SystemView::trySendOverflowPacket()
	{
		uint32_t TimeStamp;
		int32_t Delta;
		int Status;
		uint8_t aPacket[11];
		uint8_t *pPayload;

		m_packet[0] = SYSVIEW_EVTID_OVERFLOW; // 1
		pPayload = &m_packet[1];
		encodeU32(pPayload, m_dropCount);
		//
		// Compute time stamp delta and append it to packet.
		//
		TimeStamp = SEGGER_SYSVIEW_GET_TIMESTAMP();
		Delta = TimeStamp - m_lastTxTimestamp;
		encodeU32(pPayload, Delta);
		//
		// Try to store packet in RTT buffer and update time stamp when this was successful
		//
		Status = SEGGER_RTT_WriteSkipNoLock(SEGGER_SYSVIEW_RTT_CHANNEL, aPacket, pPayload - aPacket);
		if (Status)
		{
			m_lastTxTimestamp = TimeStamp;
			m_enableState--; // EnableState has been 2, will be 1. Always.
		}
		else
		{
			m_dropCount++;
		}
		//
		return Status;
	}

	/*********************************************************************
*
*       _SendPacket()
*
*  Function description
*    Send a SystemView packet over RTT. RTT channel and mode are
*    configured by macros when the SystemView component is initialized.
*    This function takes care of maintaining the packet drop count
*    and sending overflow packets when necessary.
*    The packet must be passed without Id and Length because this
*    function prepends it to the packet before transmission.
*
*  Parameters
*    pStartPacket - Pointer to start of packet payload.
*                   There must be at least 4 bytes free to prepend Id and Length.
*    pEndPacket   - Pointer to end of packet payload.
*    EventId      - Id of the event to send.
*
*/
	void SystemView::sendPacket(uint8_t *startPacket, uint8_t *endPacket, unsigned int EventId)
	{
		unsigned int NumBytes;
		uint32_t TimeStamp;
		uint32_t Delta;
		int Status;
		if (m_enableState == 1)
		{ // Enabled, no dropped packets remaining
			goto Send;
		}
		if (m_enableState == 0)
		{
			goto SendDone;
		}
		//
		// Handle buffer full situations:
		// Have packets been dropped before because buffer was full?
		// In this case try to send and overflow packet.
		//
		if (m_enableState == 2)
		{
			trySendOverflowPacket();
			if (m_enableState != 1)
			{
				goto SendDone;
			}
		}
	Send:
		//
		// Check if event is disabled from being recorded.
		//
		if (EventId < 32)
		{
			if (m_disabledEvents & ((uint32_t)1u << EventId))
			{
				goto SendDone;
			}
		}
		//
		// Prepare actual packet.
		// If it is a known packet, prepend eventId only,
		// otherwise prepend packet length and eventId.
		//
		if (EventId < 24)
		{
			*--startPacket = EventId;
		}
		else
		{
			NumBytes = endPacket - startPacket;
			if (NumBytes > 127)
			{
				*--startPacket = (NumBytes >> 7);
				*--startPacket = NumBytes | 0x80;
			}
			else
			{
				*--startPacket = NumBytes;
			}
			if (EventId > 127)
			{
				*--startPacket = (EventId >> 7);
				*--startPacket = EventId | 0x80;
			}
			else
			{
				*--startPacket = EventId;
			}
		}
		//
		// Compute time stamp delta and append it to packet.
		//
		TimeStamp = SEGGER_SYSVIEW_GET_TIMESTAMP();
		Delta = TimeStamp - m_lastTxTimestamp;
		encodeU32(endPacket, Delta);
		//
		// Try to store packet in RTT buffer and update time stamp when this was successful
		//
		Status = SEGGER_RTT_WriteSkipNoLock(SEGGER_SYSVIEW_RTT_CHANNEL, startPacket, endPacket - startPacket);
		if (Status)
		{
			m_lastTxTimestamp = TimeStamp;
		}
		else
		{
			m_enableState++; // EnableState has been 1, will be 2. Always.
		}

	SendDone:
		//
		// Check if host is sending data which needs to be processed.
		// Note that since this code is called for every packet, it is very time critical, so we do
		// only what is really needed here, which is checking if there is any data
		//
		if (rttHasData(SEGGER_SYSVIEW_RTT_CHANNEL))
		{
			if (m_recurtionCounter == 0)
			{ // Avoid uncontrolled nesting. This way, this routine can call itself once, but no more often than that.
				m_recurtionCounter = 1;
				handleIncomingPacket();
				m_recurtionCounter = 0;
			}
		}
	}

	/*********************************************************************
*
*       _VPrintHost()
*
*  Function description
*    Send a format string and its parameters to the host.
*
*  Parameters
*    s            Pointer to format string.
*    Options      Options to be sent to the host.
*    pParamList   Pointer to the list of arguments for the format string.
*/
	int SystemView::vPrintHost(const char *s, uint32_t options, va_list *paramList)
	{
		uint32_t aParas[SEGGER_SYSVIEW_MAX_ARGUMENTS];
		uint32_t *pParas;
		uint32_t NumArguments;
		const char *p;
		char c;
		uint8_t *pPayload;
		uint8_t *pPayloadStart;
		//
		// Count number of arguments by counting '%' characters in string.
		// If enabled, check for non-scalar modifier flags to format string on the target.
		//
		p = s;
		NumArguments = 0;
		for (;;)
		{
			c = *p++;
			if (c == 0)
			{
				break;
			}
			if (c == '%')
			{
				c = *p;
				aParas[NumArguments++] = va_arg(*paramList, int);
				if (NumArguments == SEGGER_SYSVIEW_MAX_ARGUMENTS)
				{
					break;
				}
			}
		}
		//
		// Send string and parameters to host
		//
		{
			recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_MAX_STRING_LEN + 2 * SEGGER_SYSVIEW_QUANTA_U32 + SEGGER_SYSVIEW_MAX_ARGUMENTS * SEGGER_SYSVIEW_QUANTA_U32);
			pPayload = encodeString(pPayloadStart, s, SEGGER_SYSVIEW_MAX_STRING_LEN);
			encodeU32(pPayload, options, NumArguments);
			pParas = aParas;
			while (NumArguments--)
			{
				encodeU32(pPayload, (*pParas));
				pParas++;
			}
			sendPacket(pPayloadStart, pPayload, SYSVIEW_EVTID_PRINT_FORMATTED);
			recordEnd();
		}
		return 0;
	}

/*********************************************************************
*
*       _StoreChar()
*
*  Function description
*    Stores a character in the printf-buffer and sends the buffer when
*     it is filled.
*
*  Parameters
*    p            Pointer to the buffer description.
*    c            Character to be printed.
*/
	void SystemView::storeChar(SysviewPrintfDesc &p, char c)
	{
		unsigned int Cnt;
		uint8_t *pPayload;
		uint32_t Options;

		Cnt = p.m_counter;
		if ((Cnt + 1u) <= SEGGER_SYSVIEW_MAX_STRING_LEN)
		{
			*(p.m_payload++) = c;
			p.m_counter = Cnt + 1u;
		}
		//
		// Write part of string, when the buffer is full
		//
		if (p.m_counter == SEGGER_SYSVIEW_MAX_STRING_LEN)
		{
			*(p.m_payloadStart) = p.m_counter;
			pPayload = p.m_payload;
			Options = p.m_options;
			encodeU32(pPayload, Options, 0);
			sendPacket(p.m_payloadStart, pPayload, SYSVIEW_EVTID_PRINT_FORMATTED);
			p.m_payloadStart = preparePacket(p.m_buffer);
			p.m_payload = p.m_payloadStart + 1u;
			p.m_counter = 0u;
		}
	}

	/*********************************************************************
*
*       _PrintUnsigned()
*
*  Function description
*    Print an unsigned integer with the given formatting into the 
*     formatted string.
*
*  Parameters
*    pBufferDesc  Pointer to the buffer description.
*    v            Value to be printed.
*    Base         Base of the value.
*    NumDigits    Number of digits to be printed.
*    FieldWidth   Width of the printed field.
*    FormatFlags  Flags for formatting the value.
*/
	void SystemView::print(SysviewPrintfDesc &pBufferDesc, unsigned int v, unsigned int Base, unsigned int NumDigits, unsigned int FieldWidth, unsigned int FormatFlags)
	{
		static const char _aV2C[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
		unsigned int Div;
		unsigned int Digit;
		unsigned int Number;
		unsigned int Width;
		char c;

		Number = v;
		Digit = 1u;
		//
		// Get actual field width
		//
		Width = 1u;
		while (Number >= Base)
		{
			Number = (Number / Base);
			Width++;
		}
		if (NumDigits > Width)
		{
			Width = NumDigits;
		}
		//
		// Print leading chars if necessary
		//
		if ((FormatFlags & FORMAT_FLAG_LEFT_JUSTIFY) == 0u)
		{
			if (FieldWidth != 0u)
			{
				if (((FormatFlags & FORMAT_FLAG_PAD_ZERO) == FORMAT_FLAG_PAD_ZERO) && (NumDigits == 0u))
				{
					c = '0';
				}
				else
				{
					c = ' ';
				}
				while ((FieldWidth != 0u) && (Width < FieldWidth))
				{
					FieldWidth--;
					storeChar(pBufferDesc, c);
				}
			}
		}
		//
		// Compute Digit.
		// Loop until Digit has the value of the highest digit required.
		// Example: If the output is 345 (Base 10), loop 2 times until Digit is 100.
		//
		while (1)
		{
			if (NumDigits > 1u)
			{ // User specified a min number of digits to print? => Make sure we loop at least that often, before checking anything else (> 1 check avoids problems with NumDigits being signed / unsigned)
				NumDigits--;
			}
			else
			{
				Div = v / Digit;
				if (Div < Base)
				{ // Is our divider big enough to extract the highest digit from value? => Done
					break;
				}
			}
			Digit *= Base;
		}
		//
		// Output digits
		//
		do
		{
			Div = v / Digit;
			v -= Div * Digit;
			storeChar(pBufferDesc, _aV2C[Div]);
			Digit /= Base;
		} while (Digit);
		//
		// Print trailing spaces if necessary
		//
		if ((FormatFlags & FORMAT_FLAG_LEFT_JUSTIFY) == FORMAT_FLAG_LEFT_JUSTIFY)
		{
			if (FieldWidth != 0u)
			{
				while ((FieldWidth != 0u) && (Width < FieldWidth))
				{
					FieldWidth--;
					storeChar(pBufferDesc, ' ');
				}
			}
		}
	}

	/*********************************************************************
*
*       _PrintInt()
*
*  Function description
*    Print a signed integer with the given formatting into the 
*     formatted string.
*
*  Parameters
*    pBufferDesc  Pointer to the buffer description.
*    v            Value to be printed.
*    Base         Base of the value.
*    NumDigits    Number of digits to be printed.
*    FieldWidth   Width of the printed field.
*    FormatFlags  Flags for formatting the value.
*/
	void SystemView::print(SysviewPrintfDesc &pBufferDesc, int v, unsigned int Base, unsigned int NumDigits, unsigned int FieldWidth, unsigned int FormatFlags)
	{
		unsigned int Width;
		int Number;

		Number = (v < 0) ? -v : v;

		//
		// Get actual field width
		//
		Width = 1u;
		while (Number >= (int)Base)
		{
			Number = (Number / (int)Base);
			Width++;
		}
		if (NumDigits > Width)
		{
			Width = NumDigits;
		}
		if ((FieldWidth > 0u) && ((v < 0) || ((FormatFlags & FORMAT_FLAG_PRINT_SIGN) == FORMAT_FLAG_PRINT_SIGN)))
		{
			FieldWidth--;
		}

		//
		// Print leading spaces if necessary
		//
		if ((((FormatFlags & FORMAT_FLAG_PAD_ZERO) == 0u) || (NumDigits != 0u)) && ((FormatFlags & FORMAT_FLAG_LEFT_JUSTIFY) == 0u))
		{
			if (FieldWidth != 0u)
			{
				while ((FieldWidth != 0u) && (Width < FieldWidth))
				{
					FieldWidth--;
					storeChar(pBufferDesc, ' ');
				}
			}
		}
		//
		// Print sign if necessary
		//
		if (v < 0)
		{
			v = -v;
			storeChar(pBufferDesc, '-');
		}
		else if ((FormatFlags & FORMAT_FLAG_PRINT_SIGN) == FORMAT_FLAG_PRINT_SIGN)
		{
			storeChar(pBufferDesc, '+');
		}
		else
		{
		}
		//
		// Print leading zeros if necessary
		//
		if (((FormatFlags & FORMAT_FLAG_PAD_ZERO) == FORMAT_FLAG_PAD_ZERO) && ((FormatFlags & FORMAT_FLAG_LEFT_JUSTIFY) == 0u) && (NumDigits == 0u))
		{
			if (FieldWidth != 0u)
			{
				while ((FieldWidth != 0u) && (Width < FieldWidth))
				{
					FieldWidth--;
					storeChar(pBufferDesc, '0');
				}
			}
		}
		//
		// Print number without sign
		//
		print(pBufferDesc, (unsigned int)v, Base, NumDigits, FieldWidth, FormatFlags);
	}

	/*********************************************************************
*
*       _VPrintTarget()
*
*  Function description
*    Stores a formatted string.
*    This data is read by the host.
*
*  Parameters
*    sFormat      Pointer to format string.
*    Options      Options to be sent to the host.
*    pParamList   Pointer to the list of arguments for the format string.
*/
	void SystemView::vPrintTarget(const char *sFormat, uint32_t Options, va_list *pParamList)
	{
		SysviewPrintfDesc BufferDesc;
		char c;
		int v;
		unsigned int NumDigits;
		unsigned int FormatFlags;
		unsigned int FieldWidth;
		uint8_t *pPayloadStart;
		recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + SEGGER_SYSVIEW_MAX_STRING_LEN + 1 + 2 * SEGGER_SYSVIEW_QUANTA_U32);
		BufferDesc.m_buffer = m_packet.data();
		BufferDesc.m_counter = 0u;
		BufferDesc.m_payloadStart = pPayloadStart;
		BufferDesc.m_payload = BufferDesc.m_payloadStart + 1u;
		BufferDesc.m_options = Options;

		do
		{
			c = *sFormat;
			sFormat++;
			if (c == 0u)
			{
				break;
			}
			if (c == '%')
			{
				//
				// Filter out flags
				//
				FormatFlags = 0u;
				v = 1;
				do
				{
					c = *sFormat;
					switch (c)
					{
					case '-':
						FormatFlags |= FORMAT_FLAG_LEFT_JUSTIFY;
						sFormat++;
						break;
					case '0':
						FormatFlags |= FORMAT_FLAG_PAD_ZERO;
						sFormat++;
						break;
					case '+':
						FormatFlags |= FORMAT_FLAG_PRINT_SIGN;
						sFormat++;
						break;
					case '#':
						FormatFlags |= FORMAT_FLAG_ALTERNATE;
						sFormat++;
						break;
					default:
						v = 0;
						break;
					}
				} while (v);
				//
				// filter out field with
				//
				FieldWidth = 0u;
				do
				{
					c = *sFormat;
					if ((c < '0') || (c > '9'))
					{
						break;
					}
					sFormat++;
					FieldWidth = (FieldWidth * 10u) + ((unsigned int)c - '0');
				} while (1);

				//
				// Filter out precision (number of digits to display)
				//
				NumDigits = 0u;
				c = *sFormat;
				if (c == '.')
				{
					sFormat++;
					do
					{
						c = *sFormat;
						if ((c < '0') || (c > '9'))
						{
							break;
						}
						sFormat++;
						NumDigits = NumDigits * 10u + ((unsigned int)c - '0');
					} while (1);
				}
				//
				// Filter out length modifier
				//
				c = *sFormat;
				do
				{
					if ((c == 'l') || (c == 'h'))
					{
						c = *sFormat;
						sFormat++;
					}
					else
					{
						break;
					}
				} while (1);
				//
				// Handle specifiers
				//
				switch (c)
				{
				case 'c':
				{
					char c0;
					v = va_arg(*pParamList, int);
					c0 = (char)v;
					storeChar(BufferDesc, c0);
					break;
				}
				case 'd':
					v = va_arg(*pParamList, int);
					print(BufferDesc, v, 10u, NumDigits, FieldWidth, FormatFlags);
					break;
				case 'u':
					v = va_arg(*pParamList, int);
					print(BufferDesc, (unsigned int)v, 10u, NumDigits, FieldWidth, FormatFlags);
					break;
				case 'x':
				case 'X':
					v = va_arg(*pParamList, int);
					print(BufferDesc, (unsigned int)v, 16u, NumDigits, FieldWidth, FormatFlags);
					break;
				case 'p':
					v = va_arg(*pParamList, int);
					print(BufferDesc, (unsigned int)v, 16u, 8u, 8u, 0u);
					break;
				case '%':
					storeChar(BufferDesc, '%');
					break;
				default:
					break;
				}
				sFormat++;
			}
			else
			{
				storeChar(BufferDesc, c);
			}
		} while (*sFormat);

		//
		// Write remaining data, if any
		//
		if (BufferDesc.m_counter != 0u)
		{
			*(BufferDesc.m_payloadStart) = BufferDesc.m_counter;
			encodeU32(BufferDesc.m_payload, BufferDesc.m_options, 0);
			sendPacket(BufferDesc.m_payloadStart, BufferDesc.m_payload, SYSVIEW_EVTID_PRINT_FORMATTED);
		}
		recordEnd();
	}

	inline void SystemView::encodeU32(uint8_t* &dest, uint32_t value)
	{
		uint8_t *pSysviewPointer;
		uint32_t SysViewData;
		pSysviewPointer = dest;
		SysViewData = value;
		while (SysViewData > 0x7F)
		{
			*pSysviewPointer++ = (uint8_t)(SysViewData | 0x80);
			SysViewData >>= 7;
		};
		*pSysviewPointer++ = (uint8_t)SysViewData;
		dest = pSysviewPointer;
	}


	inline void SystemView::recordStart(uint8_t *&payloadStart, uint16_t packetSize)
	{
		m_lockLevel = lock(SEGGER_RTT_MAX_INTERRUPT_PRIORITY);
		payloadStart = preparePacket(m_packet.data());
	}

	inline void SystemView::recordEnd()
	{
		unlock(m_lockLevel); // restore priorityLevel
	}
	inline uint32_t SystemView::rttHasData(uint16_t n)
	{
		return (_SEGGER_RTT.aDown[n].WrOff - _SEGGER_RTT.aDown[n].RdOff);
	}
	
}