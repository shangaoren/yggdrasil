#pragma once

#include <array>

#include "yggdrasil/kernel/Processor.hpp"
#include "yggdrasil/kernel/Core.hpp"
#include "yggdrasil/kernel/Task.hpp"
#include "Config/SEGGER_SYSVIEW_Conf.h"
#include "SEGGER/SEGGER_SYSVIEW_ConfDefaults.h"
#include "SEGGER/SEGGER_SYSVIEW_Int.h"

#include "SEGGER/SEGGER_RTT.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define SEGGER_SYSVIEW_VERSION 21000

#define SEGGER_SYSVIEW_INFO_SIZE 9  // Minimum size, which has to be reserved for a packet. 1-2 byte of message type, 0-2  byte of payload length, 1-5 bytes of timestamp.
#define SEGGER_SYSVIEW_QUANTA_U32 5 // Maximum number of bytes to encode a U32, should be reserved for each 32-bit value in a packet.

#define SEGGER_SYSVIEW_LOG (0u)
#define SEGGER_SYSVIEW_WARNING (1u)
#define SEGGER_SYSVIEW_ERROR (2u)
#define SEGGER_SYSVIEW_FLAG_APPEND (1u << 6)

#define SEGGER_SYSVIEW_PREPARE_PACKET(p) (p) + 4
//
// SystemView events. First 32 IDs from 0 .. 31 are reserved for these
//
#define SYSVIEW_EVTID_NOP 0 // Dummy packet.
#define SYSVIEW_EVTID_OVERFLOW 1
#define SYSVIEW_EVTID_ISR_ENTER 2
#define SYSVIEW_EVTID_ISR_EXIT 3
#define SYSVIEW_EVTID_TASK_START_EXEC 4
#define SYSVIEW_EVTID_TASK_STOP_EXEC 5
#define SYSVIEW_EVTID_TASK_START_READY 6
#define SYSVIEW_EVTID_TASK_STOP_READY 7
#define SYSVIEW_EVTID_TASK_CREATE 8
#define SYSVIEW_EVTID_TASK_INFO 9
#define SYSVIEW_EVTID_TRACE_START 10
#define SYSVIEW_EVTID_TRACE_STOP 11
#define SYSVIEW_EVTID_SYSTIME_CYCLES 12
#define SYSVIEW_EVTID_SYSTIME_US 13
#define SYSVIEW_EVTID_SYSDESC 14
#define SYSVIEW_EVTID_USER_START 15
#define SYSVIEW_EVTID_USER_STOP 16
#define SYSVIEW_EVTID_IDLE 17
#define SYSVIEW_EVTID_ISR_TO_SCHEDULER 18
#define SYSVIEW_EVTID_TIMER_ENTER 19
#define SYSVIEW_EVTID_TIMER_EXIT 20
#define SYSVIEW_EVTID_STACK_INFO 21
#define SYSVIEW_EVTID_MODULEDESC 22

#define SYSVIEW_EVTID_INIT 24
#define SYSVIEW_EVTID_NAME_RESOURCE 25
#define SYSVIEW_EVTID_PRINT_FORMATTED 26
#define SYSVIEW_EVTID_NUMMODULES 27
#define SYSVIEW_EVTID_END_CALL 28
#define SYSVIEW_EVTID_TASK_TERMINATE 29

#define SYSVIEW_EVTID_EX 31
//
// Event masks to disable/enable events
//
#define SYSVIEW_EVTMASK_NOP (1 << SYSVIEW_EVTID_NOP)
#define SYSVIEW_EVTMASK_OVERFLOW (1 << SYSVIEW_EVTID_OVERFLOW)
#define SYSVIEW_EVTMASK_ISR_ENTER (1 << SYSVIEW_EVTID_ISR_ENTER)
#define SYSVIEW_EVTMASK_ISR_EXIT (1 << SYSVIEW_EVTID_ISR_EXIT)
#define SYSVIEW_EVTMASK_TASK_START_EXEC (1 << SYSVIEW_EVTID_TASK_START_EXEC)
#define SYSVIEW_EVTMASK_TASK_STOP_EXEC (1 << SYSVIEW_EVTID_TASK_STOP_EXEC)
#define SYSVIEW_EVTMASK_TASK_START_READY (1 << SYSVIEW_EVTID_TASK_START_READY)
#define SYSVIEW_EVTMASK_TASK_STOP_READY (1 << SYSVIEW_EVTID_TASK_STOP_READY)
#define SYSVIEW_EVTMASK_TASK_CREATE (1 << SYSVIEW_EVTID_TASK_CREATE)
#define SYSVIEW_EVTMASK_TASK_INFO (1 << SYSVIEW_EVTID_TASK_INFO)
#define SYSVIEW_EVTMASK_TRACE_START (1 << SYSVIEW_EVTID_TRACE_START)
#define SYSVIEW_EVTMASK_TRACE_STOP (1 << SYSVIEW_EVTID_TRACE_STOP)
#define SYSVIEW_EVTMASK_SYSTIME_CYCLES (1 << SYSVIEW_EVTID_SYSTIME_CYCLES)
#define SYSVIEW_EVTMASK_SYSTIME_US (1 << SYSVIEW_EVTID_SYSTIME_US)
#define SYSVIEW_EVTMASK_SYSDESC (1 << SYSVIEW_EVTID_SYSDESC)
#define SYSVIEW_EVTMASK_USER_START (1 << SYSVIEW_EVTID_USER_START)
#define SYSVIEW_EVTMASK_USER_STOP (1 << SYSVIEW_EVTID_USER_STOP)
#define SYSVIEW_EVTMASK_IDLE (1 << SYSVIEW_EVTID_IDLE)
#define SYSVIEW_EVTMASK_ISR_TO_SCHEDULER (1 << SYSVIEW_EVTID_ISR_TO_SCHEDULER)
#define SYSVIEW_EVTMASK_TIMER_ENTER (1 << SYSVIEW_EVTID_TIMER_ENTER)
#define SYSVIEW_EVTMASK_TIMER_EXIT (1 << SYSVIEW_EVTID_TIMER_EXIT)
#define SYSVIEW_EVTMASK_STACK_INFO (1 << SYSVIEW_EVTID_STACK_INFO)
#define SYSVIEW_EVTMASK_MODULEDESC (1 << SYSVIEW_EVTID_MODULEDESC)

#define SYSVIEW_EVTMASK_INIT (1 << SYSVIEW_EVTID_INIT)
#define SYSVIEW_EVTMASK_NAME_RESOURCE (1 << SYSVIEW_EVTID_NAME_RESOURCE)
#define SYSVIEW_EVTMASK_PRINT_FORMATTED (1 << SYSVIEW_EVTID_PRINT_FORMATTED)
#define SYSVIEW_EVTMASK_NUMMODULES (1 << SYSVIEW_EVTID_NUMMODULES)
#define SYSVIEW_EVTMASK_END_CALL (1 << SYSVIEW_EVTID_END_CALL)
#define SYSVIEW_EVTMASK_TASK_TERMINATE (1 << SYSVIEW_EVTID_TASK_TERMINATE)

#define SYSVIEW_EVTMASK_EX (1 << SYSVIEW_EVTID_EX)

#define SYSVIEW_EVTMASK_ALL_INTERRUPTS (SYSVIEW_EVTMASK_ISR_ENTER | SYSVIEW_EVTMASK_ISR_EXIT | SYSVIEW_EVTMASK_ISR_TO_SCHEDULER)
#define SYSVIEW_EVTMASK_ALL_TASKS (SYSVIEW_EVTMASK_TASK_START_EXEC | SYSVIEW_EVTMASK_TASK_STOP_EXEC | SYSVIEW_EVTMASK_TASK_START_READY | SYSVIEW_EVTMASK_TASK_STOP_READY | SYSVIEW_EVTMASK_TASK_CREATE | SYSVIEW_EVTMASK_TASK_INFO | SYSVIEW_EVTMASK_STACK_INFO | SYSVIEW_EVTMASK_TASK_TERMINATE)

#ifndef SYSVIEW_APP_NAME
#define SYSVIEW_APP_NAME "Unknown"
#endif

#ifndef SYSVIEW_DEVICE_NAME
#define SYSVIEW_DEVICE_NAME "Unknown"
#endif

namespace kernel
{

	class SysviewPrintfDesc
	{
	  public:
		uint8_t *m_buffer;
		uint8_t *m_payload;
		uint8_t *m_payloadStart;
		uint32_t m_options;
		unsigned m_counter;

	  private:
	};

	class SysviewModule
	{
	  public:
		const char *m_module;
		uint32_t m_numEvents;
		uint32_t m_eventOffset;
		void (*m_sendModuleDescription)(void);
		SysviewModule *m_next;

	  private:
	}; 
	
	class SystemView
	{
	  public:
		
		
		/* Added */
		void registerInterrupt(const char *name);

		uint8_t lock(uint8_t lockLevel);
		void unlock(uint8_t lockLevel);

		static void svcBootstrap();
		static void SysviewIsrHandler();

		static void pendSvHandler();

		/*********************************************************************
*
*       Control and initialization functions
*/
		void init(kernel::Core &systemCore);
		void start();
		void stop();
		void systemDescription();
		void sendTaskList();
		void sendTaskInfo(const kernel::Task* task);
		void sendSystemDescription(const char *systemDescription);
		int16_t isStarted();

/*********************************************************************
*
*       Event recording functions
*/
		void record(unsigned int EventId);

/*********************************************************************
*
*       SEGGER_SYSVIEW_RecordU32()
*
*  Function description
*    Formats and sends a SystemView packet containing a single U32
*    parameter payload.
*
*  Parameters
*    EventID - SystemView event ID.
*    Value   - The 32-bit parameter encoded to SystemView packet payload.
*/
		template <class... Args>
		void record(unsigned int EventId, Args... args)
		{
			uint8_t *pPayload;
			uint8_t *pPayloadStart;
			recordStart(pPayloadStart, SEGGER_SYSVIEW_INFO_SIZE + (sizeof...(args))*SEGGER_SYSVIEW_QUANTA_U32);
			pPayload = pPayloadStart;
			encodeU32(pPayload, args...);
			sendPacket(pPayloadStart, pPayload, EventId);
			recordEnd();
		}
		
		void record(unsigned int EventId, const char* string);
		void recordSystemTime();
		void recordEnterIsr();
		void recordExitIsr();
		void recordIsrToScheduler();
		void recordEnterTimer(uint32_t timerId);
		void recordExitTimer();
		void recordEndCall(unsigned int EventId);
		void recordEndCall(unsigned int EventId, uint32_t Param);

		void onIdle();
		void onTaskCreate(const kernel::Task* task);
		void onTaskTerminate(const kernel::Task *task);
		void onTaskStartExec(const kernel::Task *task);
		void onTaskStopExec();
		void onTaskStartReady(const kernel::Task *task);
		void onTaskStopReady(const kernel::Task *task, unsigned int cause);
		void onUserStart(unsigned int UserId);
		void onUserStop(unsigned int USerId);

		void nameRessource(uint32_t resourceId, const char *name);
		uint16_t sendPacketWithLock(uint8_t* packet, uint8_t *payloadEnd, unsigned int EventId);

		/*********************************************************************
*
*       Middleware module registration
*/
		void registerModule(SysviewModule &Module);
		void recordModuleDescription(const SysviewModule &Module, const char *Description);
		void sendModule(uint8_t ModuleId);
		void sendModuleDescription(void);
		void sendNumModules(void);

/*********************************************************************
*
*       printf-Style functions
*/
		void printfHostEx(const char *s, uint32_t Options, ...);
		void printfTargetEx(const char *s, uint32_t Options, ...);
		void printfHost(const char *s, ...);
		void printfTarget(const char *s, ...);
		void warnfHost(const char *s, ...);
		void warnfTarget(const char *s, ...);
		void errorfHost(const char *s, ...);
		void errorfTarget(const char *s, ...);


		void print(const char *s);
		void warn(const char *s);
		void error(const char *s);

		void enableEvents(uint32_t enableMask);
		void disableEvents(uint32_t disableMask);

		static SystemView &get()
		{
			return s_self;
		}

	  private:
		
		/*-------------- Variables------------------------*/
		static SystemView s_self;
		kernel::Core *m_core;
		const std::array<uint8_t,10> Sync = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		uint32_t m_ramBaseAddress = 0x20000000;
		int16_t m_enableState = 0;
		uint32_t m_disabledEvents = 0;
		uint32_t m_recurtionCounter = 0;
		SysviewModule *m_first = nullptr;
		uint32_t m_moduleCount = 0;
		uint32_t m_dropCount = 0;
		uint32_t m_lastTxTimestamp = 0;
		uint8_t m_lockLevel = 0;

		std::array<char, SEGGER_SYSVIEW_RTT_BUFFER_SIZE> m_upBuffer;
		std::array<char, 8> m_downBuffer;
		std::array<uint8_t, SEGGER_SYSVIEW_MAX_PACKET_SIZE> m_packet;

		/* ------------------------------Private Functions --------------------------*/
		uint8_t *encode(uint8_t* payload, uint32_t value);
		uint8_t *encode(uint8_t *payload, const char *src, unsigned int len);
		uint8_t *encodeString(uint8_t *payload, const char *string, unsigned int maxLen);
		uint8_t *encodeId(uint8_t *payload, uint32_t id);
		uint32_t shrinkId(uint32_t id);

		uint8_t *preparePacket(uint8_t *packet);
		void handleIncomingPacket();
		int trySendOverflowPacket();
		void sendPacket(uint8_t* startPacket, uint8_t* endPacket, unsigned int EventId);
		int vPrintHost(const char *string, uint32_t options, va_list *paramList);
		void storeChar(SysviewPrintfDesc &p, char c);
		void print(SysviewPrintfDesc &pBufferDesc, unsigned int v, unsigned int Base, unsigned int NumDigits, unsigned int FieldWidth, unsigned int FormatFlags);
		void print(SysviewPrintfDesc &pBufferDesc, int v, unsigned int Base, unsigned int NumDigits, unsigned int FieldWidth, unsigned int FormatFlags);
		void vPrintTarget(const char *sFormat, uint32_t Options, va_list *pParamList);
		inline void encodeU32(uint8_t* &dest, uint32_t value);
		template <class... Args>
		inline void encodeU32(uint8_t *&dest, uint32_t param0, Args... args)
		{
			encodeU32(dest, param0);
			encodeU32(dest, args...);
		}
		inline void recordStart(uint8_t *&payloadStart, uint16_t packetSize);
		inline void recordEnd();
		inline uint32_t rttHasData(uint16_t n);
	};
}