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
#include "yggdrasil/framework/DualLinkedList.hpp"
#include "yggdrasil/interfaces/IWaitable.hpp"


namespace kernel
{
	class Task;
	
	class StartedList : public framework::DualLinkedList<Task, StartedList>
	{};
	class ReadyList : public framework::DualLinkedList<Task, ReadyList>
	{};
	class SleepingList : public framework::DualLinkedList<Task, SleepingList>
	{};
	class WaitableList : public framework::DualLinkedList<Task, WaitableList>
	{};
	class EventList : public framework::DualLinkedList<Task, EventList>
	{};

	class Task : public framework::DualLinkNode<Task, StartedList>,
				 public framework::DualLinkNode<Task, ReadyList>,
				 public framework::DualLinkNode<Task, SleepingList>,
				 public framework::DualLinkNode<Task, WaitableList>,
				 public framework::DualLinkNode<Task,EventList>
	{
		friend class Scheduler;
		friend class Event;
		friend class Mutex;
		friend class SystemView;
	public:
	  
	  using TaskFunc =  void (*)(uint32_t);
		using StartTaskStub = bool(&)(Task*);
		

	  virtual bool start();
		static StartTaskStub& startTaskStub;
	  virtual bool stop();
	  bool isStackCorrupted();
	  static void taskFinished();

	  enum class state : uint32_t
	  {
		  sleeping = 0,
		  active = 1,
		  waitingEvent = 2,
		  notStarted = 3,
		  ready = 4,
		  waitingMutex = 5,
	  };

	  constexpr Task(uint32_t* stack, uint32_t stackSize, TaskFunc function, uint32_t taskPriority, uint32_t parameter = 0, const char *name = "undefined")
		  : m_stackPointer(stack + stackSize - 18), //move pointer to bottom of stack (higher @) and reserve place to hold 8 + 10 registers
			m_stackOrigin(stack),
			m_stackSize(stackSize),
			m_wakeUpTimeStamp(0),
			m_mainFunction(function),
			m_taskPriority(taskPriority),
			m_isPrivilegied(false),
			m_started(false),
			m_state(state::notStarted),
			m_name(name)
#ifdef KDEBUG
			,m_stackUsage(0)
#endif
		{
			m_stackOrigin[0] = 0xDEAD;
			m_stackOrigin[1] = 0xBEEF;
			m_stackOrigin[2] = 0xDEAD;
			m_stackOrigin[3] = 0xBEEF;
			m_stackOrigin[4] = 0xDEAD;
			m_stackOrigin[5] = 0xBEEF;
			m_stackOrigin[6] = 0xDEAD;
			m_stackOrigin[7] = 0xBEEF;
			
			//stacked by hardware
			m_stackPointer[17] = 0x01000000;								 //initial xPSR
			m_stackPointer[16] = reinterpret_cast<uint32_t>(m_mainFunction); //PC
			m_stackPointer[15] = reinterpret_cast<uint32_t>(taskFinished);   //LR
			m_stackPointer[14] = 4;											 //R12
			m_stackPointer[13] = 3;											 //R3
			m_stackPointer[12] = 2;											 //R2
			m_stackPointer[11] = 1;											 //R1
			m_stackPointer[10] = parameter;									 //R0
			// software stacked registers
			m_stackPointer[9] = 11;						//R11
			m_stackPointer[8] = 10;						//R10
			m_stackPointer[7] = 9;						//R9
			m_stackPointer[6] = 8;						//R8
			m_stackPointer[5] = 7;						//R7
			m_stackPointer[4] = 6;						//R6
			m_stackPointer[3] = 5;						//R5
			m_stackPointer[2] = 4;						//R4
			m_stackPointer[1] = 0x2 | !m_isPrivilegied; //CONTROL, initial value, unprivileged, use PSP, no Floating Point
			m_stackPointer[0] = 0xFFFFFFFD;				//LR, return from exception, 8 Word Stack Length (no floating point), return in thread mode, use PSP

		}

		constexpr Task(uint32_t* stack, uint32_t stackSize, TaskFunc function, uint32_t taskPriority, uint32_t parameter, bool isPrivilegied, const char *name)
			: m_stackPointer(stack + stackSize - 19) //move pointer to bottom of stack (higher @) and reserve place to hold 8 + 10 registers
			, m_stackOrigin(stack)
			, m_stackSize(stackSize)
			, m_wakeUpTimeStamp(0)
			, m_mainFunction(function)
			, m_taskPriority(taskPriority)
			, m_isPrivilegied(isPrivilegied)
			, m_started(false)
			, m_state(state::notStarted)
			, m_name(name)
#ifdef KDEBUG
			,m_stackUsage(0)
#endif
		{
			m_stackOrigin[0] = 0xDEAD;
			m_stackOrigin[1] = 0xBEEF;
			m_stackOrigin[2] = 0xDEAD;
			m_stackOrigin[3] = 0xBEEF;
			m_stackOrigin[4] = 0xDEAD;
			m_stackOrigin[5] = 0xBEEF;
			m_stackOrigin[6] = 0xDEAD;
			m_stackOrigin[7] = 0xBEEF;

			//stacked by hardware
			m_stackPointer[17] = 0x01000000;								 //initial xPSR
			m_stackPointer[16] = reinterpret_cast<uint32_t>(m_mainFunction); //PC
			m_stackPointer[15] = reinterpret_cast<uint32_t>(taskFinished);   //LR
			m_stackPointer[14] = 4;											 //R12
			m_stackPointer[13] = 3;											 //R3
			m_stackPointer[12] = 2;											 //R2
			m_stackPointer[11] = 1;											 //R1
			m_stackPointer[10] = parameter;									 //R0
			// software stacked registers
			m_stackPointer[9] = 11;						//R11
			m_stackPointer[8] = 10;						//R10
			m_stackPointer[7] = 9;						//R9
			m_stackPointer[6] = 8;						//R8
			m_stackPointer[5] = 7;						//R7
			m_stackPointer[4] = 6;						//R6
			m_stackPointer[3] = 5;						//R5
			m_stackPointer[2] = 4;						//R4
			m_stackPointer[1] = 0x2 | !m_isPrivilegied; //CONTROL, initial value, unprivileged, use PSP, no Floating Point
			m_stackPointer[0] = 0xFFFFFFFD;				//LR, return from exception, 8 Word Stack Length (no floating point), return in thread mode, use PSP

		}
		
	private:
		uint32_t volatile* m_stackPointer;
		uint32_t* m_stackOrigin;
		const uint32_t m_stackSize;

		volatile uint32_t m_wakeUpTimeStamp;
		interfaces::IWaitable* volatile m_waitingFor = nullptr;
		TaskFunc m_mainFunction;
		uint32_t m_taskPriority;
		bool m_isPrivilegied;
		bool m_started;
		state m_state;
		const char* m_name;
#ifdef KDEBUG
		uint32_t m_stackUsage; // used to measure the usage of task's stack
#endif // KDEBUG

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

		void setReturnValue(uint32_t value)
		{
			auto ctrl = *(m_stackPointer + 8);
			if ((ctrl & 0b100) == 0) // check bit #2 of control to know if floating point is active or not
				*(reinterpret_cast<volatile uint32_t *>(m_stackPointer + 10)) = value;
			else
				*(reinterpret_cast<volatile uint32_t *>(m_stackPointer + 26)) = value; //TODO Test
		}

		void setReturnValue(int16_t value)
		{
			uint32_t ctrl = *(m_stackPointer +1);
			if ((ctrl & 0b100) == 0) // check bit #2 of control to know if floating point is active or not
				*(reinterpret_cast<volatile int16_t *>(m_stackPointer + 10)) = value;
			else
				*(reinterpret_cast<volatile int16_t *>(m_stackPointer + 26)) = value; //TODO Test
		}

		inline void setStackPointer(uint32_t* stackPosition)
		{
			m_stackPointer = stackPosition;
#ifdef KDEBUG
			m_stackUsage = m_stackSize - (stackPosition-m_stackOrigin);
#endif // KDEBUG
		}
	}; 
	
	template<uint32_t StackSize>
		class TaskWithStack : public Task
		{
		public :
			constexpr TaskWithStack<StackSize>(TaskFunc function, uint32_t taskPriority, uint32_t parameter = 0, const char* name = "undefined") :
			Task(m_stack, StackSize, function, taskPriority,parameter, name)
			{
			}
			
			constexpr TaskWithStack<StackSize>(TaskFunc function, uint32_t taskPriority, uint32_t parameter = 0,bool isPrivilegied = false, const char* name = "undefined") :
			Task(m_stack, StackSize, function, taskPriority, parameter,isPrivilegied, name)
			{
			}
			
		private :
			uint32_t m_stack[StackSize]__attribute__((aligned(4))); 
			
		};
} // namespace kernel
