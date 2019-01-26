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


#include "ServiceCall.hpp"
#include "Processor.hpp"
#include "yggdrasil/framework/DualLinkedList.hpp"

#ifdef SYSVIEW
#include "yggdrasil/systemview/segger/SEGGER_SYSVIEW.h"
#endif

namespace kernel
{
	class Task;
	
	class StartedList : public framework::DualLinkedList<Task, StartedList>
	{};
	
	class ReadyList : public framework::DualLinkedList<Task, ReadyList>
	{};
	
	class SleepingList : public framework::DualLinkedList<Task, SleepingList>
	{};
	
	class EventList : public framework::DualLinkedList<Task, EventList>
	{};
	
	class Task : 
		public framework::DualLinkNode<Task,StartedList>,
		public framework::DualLinkNode<Task,ReadyList>,
		public framework::DualLinkNode<Task,SleepingList>,
		public framework::DualLinkNode<Task,EventList>
	{
		friend class Scheduler;
		friend class Event;
		friend class SystemView;
	public:
		typedef void(*TaskFunc)(uint32_t);
		
		virtual bool start();
		static bool startTaskStub(Task* task);
		virtual bool stop();
		static void taskFinished();

		enum class state : uint32_t
		{
			sleeping = 0,
			active = 1,
			waiting = 2,
			notStarted = 3,
			ready = 4,
		};
		
		constexpr Task(uint32_t stack[], uint32_t stackSize, TaskFunc function, uint32_t taskPriority, const char* name = "undefined", uint32_t parameter = 0)
			: 
			m_stackPointer(stack),
			m_stackOrigin(stack),
			m_wakeUpTimeStamp(0),
			m_mainFunction(function),
			m_taskPriority(taskPriority),
			m_started(false),
			m_state(state::notStarted),
			m_name(name)
#ifdef SYSVIEW
			,m_info({0,nullptr,0,0,0})
#endif
		{
			m_stackPointer += (stackSize - 8); //move pointer to bottom of stack (higher @) and reserve place to hold 8 register
			//stacked by hardware
			m_stackPointer[7] = 0x01000000; //initial xPSR
			m_stackPointer[6] = reinterpret_cast<uint32_t>(m_mainFunction);//PC
			m_stackPointer[5] = reinterpret_cast<uint32_t>(taskFinished);  //LR
			m_stackPointer[4] = 0;			//R12
			m_stackPointer[3] = 0;			//R3
			m_stackPointer[2] = 0;			//R2
			m_stackPointer[1] = 0;			//R1
			m_stackPointer[0] = parameter;	//R0
			
			//stacked by software
			m_stackPointer -= 10;	//move to add 10 software stacked registers
			m_stackPointer[9] = 0;	//R11
			m_stackPointer[8] = 0;	//R10
			m_stackPointer[7] = 0;	//R9
			m_stackPointer[6] = 0;	//R8
			m_stackPointer[5] = 0;	//R7
			m_stackPointer[4] = 0;	//R6
			m_stackPointer[3] = 0;	//R5
			m_stackPointer[2] = 0;	//R4
			m_stackPointer[1] = 0x3;	//CONTROL, initial value, unprivileged, use PSP, no Floating Point 
			m_stackPointer[0] = 0xFFFFFFFD;	//LR, return from exception, 8 Word Stack Length (no floating point), return in thread mode, use PSP	
#ifdef SYSVIEW
			m_info.sName = m_name;
			m_info.Prio = m_taskPriority;
			m_info.StackBase = reinterpret_cast<uint32_t>(m_stackOrigin);
			m_info.TaskID = reinterpret_cast<uint32_t>(this);
			m_info.StackSize = stackSize;
#endif
			
		}
		
	private:
		


		uint32_t* m_stackPointer;
		uint32_t* m_stackOrigin;

		uint32_t m_wakeUpTimeStamp;
		void(*m_mainFunction)(uint32_t);
		
		uint32_t m_taskPriority;
		bool m_started;
		state m_state;
		const char* m_name;
#ifdef SYSVIEW
		SEGGER_SYSVIEW_TASKINFO m_info;
#endif
		
		
		/*Compare two Task timestamps
		 * if base task was running after compared result is 1
		 * if compared was running before base result is -1
		 * if timestamps are the same
		 * 		if base is higher priority result is -1
		 * 		if compared is higher priority result is 1
		 * 		else result is 0								*/
		static int8_t sleepCompare(Task* base, Task* compared)
		{
			if (base->m_wakeUpTimeStamp > compared->m_wakeUpTimeStamp)
				return 1;
			if (base->m_wakeUpTimeStamp < compared->m_wakeUpTimeStamp)
				return -1;
			if (base->m_wakeUpTimeStamp == compared->m_wakeUpTimeStamp)
			{
				if (base->m_taskPriority > compared->m_taskPriority)
					return -1;
				if (base->m_taskPriority < compared->m_taskPriority)
					return 1;
				if (base->m_taskPriority == compared->m_taskPriority)
					return 0;
			}
			return 0;
		}
		
		/*Compare two tasks
		 * If base Task has higher priority (higher number) result is -1
		 * If priorities are equals result is 0
		 * If compared has higher priority result is 1*/
		static int8_t priorityCompare(Task* base, Task* compared)
		{
			if (base->m_taskPriority > compared->m_taskPriority)
				return -1;
			if (base->m_taskPriority < compared->m_taskPriority)
				return 1;
			if (base->m_taskPriority == compared->m_taskPriority)
				return 0;
			return 0;
		}
		
	
	};
	
	template<uint32_t StackSize>
		class TaskWithStack : public Task
		{
		public :
			constexpr TaskWithStack<StackSize>(TaskFunc function, uint32_t taskPriority, const char* name = "undefined", uint32_t parameter = 0) :
			Task(m_stack, StackSize, function, taskPriority, name, parameter)
			{
			}
			
		private :
			uint32_t m_stack[StackSize]__attribute__((aligned(4))); 
			
		};
	}
