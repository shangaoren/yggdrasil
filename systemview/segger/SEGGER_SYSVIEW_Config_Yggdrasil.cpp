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

File    : SEGGER_SYSVIEW_Config_embOS.c
Purpose : Sample setup configuration of SystemView with Yggdrasil.
Revision: $Rev: 7750 $
*/
//#include "RTOS.h"
#include <cstdint>
#include "SEGGER_SYSVIEW.h"
#include "../Config/SEGGER_SYSVIEW_Conf.h"
#include "SEGGER_SYSVIEW_Yggdrasil.hpp"

//
// SystemcoreClock can be used in most CMSIS compatible projects.
// In non-CMSIS projects define SYSVIEW_CPU_FREQ.
//
extern uint32_t SystemCoreClock;

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
// The application name to be displayed in SystemViewer
#ifndef   SYSVIEW_APP_NAME
#define SYSVIEW_APP_NAME        "Unknown Application"
#endif

// The target device name
#ifndef   SYSVIEW_DEVICE_NAME
#define SYSVIEW_DEVICE_NAME     "Cortex-M4"
#endif

// Frequency of the timestamp. Must match SEGGER_SYSVIEW_Conf.h
#ifndef   SYSVIEW_TIMESTAMP_FREQ
#define SYSVIEW_TIMESTAMP_FREQ  (SystemCoreClock)
#endif

// System Frequency. SystemcoreClock is used in most CMSIS compatible projects.
#ifndef   SYSVIEW_CPU_FREQ
#define SYSVIEW_CPU_FREQ        (SystemCoreClock)
#endif

// The lowest RAM address used for IDs (pointers)
#ifndef   SYSVIEW_RAM_BASE
#define SYSVIEW_RAM_BASE        (0x20000000)
#endif

#ifndef   SYSVIEW_SYSDESC0
#define SYSVIEW_SYSDESC0        "I#11=ServiceCall"
#endif

// Define as 1 if the Cortex-M cycle counter is used as SystemView timestamp. Must match SEGGER_SYSVIEW_Conf.h
#ifndef   USE_CYCCNT_TIMESTAMP
#define USE_CYCCNT_TIMESTAMP    1
#endif

// Define as 1 if the Cortex-M cycle counter is used and there might be no debugger attached while recording.
#ifndef   ENABLE_DWT_CYCCNT
#define ENABLE_DWT_CYCCNT       (USE_CYCCNT_TIMESTAMP & SEGGER_SYSVIEW_POST_MORTEM_MODE)
#endif

//#ifndef   SYSVIEW_SYSDESC1
//  #define SYSVIEW_SYSDESC1      ""
//#endif

//#ifndef   SYSVIEW_SYSDESC2
//  #define SYSVIEW_SYSDESC2      ""
//#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/
#define DEMCR                     (*(volatile U32*) (0xE000EDFCuL))  // Debug Exception and Monitor Control Register
#define TRACEENA_BIT              (1uL << 24)                           // Trace enable bit
#define DWT_CTRL                  (*(volatile U32*) (0xE0001000uL))  // DWT Control Register
#define NOCYCCNT_BIT              (1uL << 25)                           // Cycle counter support bit
#define CYCCNTENA_BIT             (1uL << 0)                            // Cycle counter enable bit

/*********************************************************************
*
*       _cbSendSystemDesc()
*
*  Function description
*    Sends SystemView description strings.
*/
static void _cbSendSystemDesc(void) {
	SEGGER_SYSVIEW_SendSysDesc("N=" SYSVIEW_APP_NAME ",O=Yggdrasil,D=" SYSVIEW_DEVICE_NAME);
	SEGGER_SYSVIEW_SendSysDesc("I#2=NonMaskableInt");
	SEGGER_SYSVIEW_SendSysDesc("I#3=HardFault");
	SEGGER_SYSVIEW_SendSysDesc("I#4=MemoryManagement");
	SEGGER_SYSVIEW_SendSysDesc("I#5=BusFault");
	SEGGER_SYSVIEW_SendSysDesc("I#6=UsageFault");
	SEGGER_SYSVIEW_SendSysDesc("I#11=ServiceCall");
	SEGGER_SYSVIEW_SendSysDesc("I#12=DebugMonitor");
	SEGGER_SYSVIEW_SendSysDesc("I#14=PendSv");
	SEGGER_SYSVIEW_SendSysDesc("I#15=Systick");

}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/
void SEGGER_SYSVIEW_Conf(void) {
#if USE_CYCCNT_TIMESTAMP
#if ENABLE_DWT_CYCCNT
	//
	// If no debugger is connected, the DWT must be enabled by the application
	//
	if((DEMCR & TRACEENA_BIT) == 0) {
		DEMCR |= TRACEENA_BIT;
	}
#endif
	//
	//  The cycle counter must be activated in order
	//  to use time related functions.
	//
	if((DWT_CTRL & NOCYCCNT_BIT) == 0) {
		       // Cycle counter supported?
	  if((DWT_CTRL & CYCCNTENA_BIT) == 0) {
			    // Cycle counter not enabled?
	    DWT_CTRL |= CYCCNTENA_BIT;               // Enable Cycle counter
		}
	}
#endif
	SEGGER_SYSVIEW_Init(SYSVIEW_TIMESTAMP_FREQ,
		SYSVIEW_CPU_FREQ,
		&kernel::SystemView::SYSVIEW_Yggdrasil_TraceAPI,
		_cbSendSystemDesc);
	SEGGER_SYSVIEW_SetRAMBase(SYSVIEW_RAM_BASE);
	//OS_SetTraceAPI(&embOS_TraceAPI_SYSVIEW);     // Configure Yggdrasil to use SYSVIEW.
}

/*************************** End of file ****************************/
