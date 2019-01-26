/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2015 - 2017  SEGGER Microcontroller GmbH & Co. KG        *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER SystemView * Real-time application analysis           *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the RTT protocol and J-Link.                       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* conditions are met:                                                *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this list of conditions and the following disclaimer.    *
*                                                                    *
* o Redistributions in binary form must reproduce the above          *
*   copyright notice, this list of conditions and the following      *
*   disclaimer in the documentation and/or other materials provided  *
*   with the distribution.                                           *
*                                                                    *
* o Neither the name of SEGGER Microcontroller GmbH & Co. KG         *
*   nor the names of its contributors may be used to endorse or      *
*   promote products derived from this software without specific     *
*   prior written permission.                                        *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
*                                                                    *
*       SystemView version: V2.52a                                    *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : SEGGER_SYSVIEW_Yggdrasil.c
Purpose : Interface between Yggdrasil and System View.
Revision: $Rev: 7745 $
*/
#ifdef SYSVIEW
//#include "RTOS.h"

#include "SEGGER_SYSVIEW.h"
#include "SEGGER_RTT.h"
#include "SEGGER_SYSVIEW_Yggdrasil.hpp"

#include <cstdint>

using namespace kernel;
		
	void SystemView::sendTaskInfo(const kernel::Task* pTask) 
	{
			Scheduler::enterKernelCriticalSection();            // No scheduling to make sure the task list does not change while we are transmitting it
			SEGGER_SYSVIEW_SendTaskInfo(&pTask->m_info);
			Scheduler::exitCriticalSection();	// No scheduling to make sure the task list does not change while we are transmitting it
	}

		/*********************************************************************
		*
		*       OS_SYSVIEW_SendTaskList()
		*
		*  Function description
		*    This function is part of the link between embOS and SYSVIEW.
		*    Called from SystemView when asked by the host, it uses SYSVIEW
		*    functions to send the entire task list to the host.
		*/
void SystemView::sendTaskList(void) 
{
	Task* ptr = Scheduler::s_started.peekFirst();
	Scheduler::enterKernelCriticalSection(); 	// No scheduling to make sure the task list does not change while we are transmitting it
	while(ptr != nullptr)
	{
		SystemView::sendTaskInfo(ptr);
		ptr = framework::DualLinkNode<Task, StartedList>::next(ptr);
	}
	Scheduler::exitCriticalSection();             // No scheduling to make sure the task list does not change while we are transmitting it
}

		

		// embOS trace API that targets SYSVIEW
		/*const OS_TRACE_API embOS_TraceAPI_SYSVIEW = {
			//
			// Specific Trace Events
			//
			SEGGER_SYSVIEW_RecordEnterISR,
			//  void (*pfRecordEnterISR)              (void);
			SEGGER_SYSVIEW_RecordExitISR, 
			//  void (*pfRecordExitISR)               (void);
			SEGGER_SYSVIEW_RecordExitISRToScheduler,
			//  void (*pfRecordExitISRToScheduler)    (void);
			_cbSendTaskInfo,
			//  void (*pfRecordTaskInfo)              (const OS_TASK* pTask);
			_cbOnTaskCreate,
			//  void (*pfRecordTaskCreate)            (OS_U32 TaskId);
			_cbOnTaskStartExec,
			//  void (*pfRecordTaskStartExec)         (OS_U32 TaskId);
			SEGGER_SYSVIEW_OnTaskStopExec,
			//  void (*pfRecordTaskStopExec)          (void);
			_cbOnTaskStartReady,
			//  void (*pfRecordTaskStartReady)        (OS_U32 TaskId);
			_cbOnTaskStopReady, 
			//  void (*pfRecordTaskStopReady)         (OS_U32 TaskId, unsigned Reason);
			SEGGER_SYSVIEW_OnIdle, 
			//  void (*pfRecordIdle)                  (void);
			//
			// Generic Trace Event logging
			//
			SEGGER_SYSVIEW_RecordVoid,
			//  void    (*pfRecordVoid)               (unsigned Id);
			_cbRecordU32,
			//  void    (*pfRecordU32)                (unsigned Id, OS_U32 Para0);
			_cbRecordU32x2,
			//  void    (*pfRecordU32x2)              (unsigned Id, OS_U32 Para0, OS_U32 Para1);
			_cbRecordU32x3, 
			//  void    (*pfRecordU32x3)              (unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2);
			_cbRecordU32x4,
			//  void    (*pfRecordU32x4)              (unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2, OS_U32 Para3);
			_ShrinkId, 
			//  OS_U32  (*pfPtrToId)                  (OS_U32 Ptr);

		};*/

		// Services provided to SYSVIEW by Yggdrasil
	const SEGGER_SYSVIEW_OS_API SystemView::SYSVIEW_Yggdrasil_TraceAPI = {
			Scheduler::getTicks,
			sendTaskList,
		};
#endif

/*************************** End of file ****************************/
