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

namespace kernel {
class TaskController;

class StartedList: public framework::DualLinkedList<TaskController, StartedList> {
};
class ReadyList: public framework::DualLinkedList<TaskController, ReadyList> {
};
class SleepingList: public framework::DualLinkedList<TaskController, SleepingList> {
};
class WaitableList: public framework::DualLinkedList<TaskController, WaitableList> {
};
class EventList: public framework::DualLinkedList<TaskController, EventList> {
};

class TaskController: public framework::DualLinkNode<TaskController, StartedList>, public framework::DualLinkNode<TaskController, ReadyList>, public framework::DualLinkNode<TaskController, SleepingList>, public framework::DualLinkNode<TaskController, WaitableList>, public framework::DualLinkNode<TaskController, EventList> {
	friend class Scheduler;
	friend class Event;
	friend class Mutex;
	friend class SystemView;
public:

	using TaskFunc = void (*)(uint32_t);
	using StartTaskStub = volatile bool(&)(TaskController*);
	using StopTaskStub = volatile bool(&)(TaskController*);

	bool start(TaskFunc function, bool isPrivilegied, uint32_t taskPriority, uint32_t parameter, const char *name);
	bool stop();
	[[nodiscard]] bool isStackCorrupted() const;
	static StartTaskStub &startTaskStub;
	static StopTaskStub &stopTaskStub;
	static void taskWrapper(TaskController &task, TaskFunc func, uint32_t parameter);
	static void taskFinished();

	enum class State : uint32_t {
		sleeping = 0, active = 1, waitingEvent = 2, notStarted = 3, ready = 4, waitingMutex = 5,
	};
	constexpr TaskController(uint32_t *stack,const uint32_t stackSize) :
		m_stackOrigin(stack), m_stackSize(stackSize) {
	}

private:
	uint32_t volatile *m_stackPointer = nullptr;
	uint32_t *const m_stackOrigin;
	const uint32_t m_stackSize;

	volatile uint32_t m_wakeUpTimeStamp = 0;
	interfaces::IWaitable *volatile m_waitingFor = nullptr;
	uint32_t m_priority = 0;
	State m_state = State::notStarted;
	const char *m_name = nullptr;
#ifdef KDEBUG
	//TODO restore
		//uint32_t m_stackUsage; // used to measure the usage of task's stack
#endif // KDEBUG


	/*Compare two Task timestamps
	 * if base task was running after compared result is 1
	 * if compared was running before base result is -1
	 * if timestamps are the same
	 * 		if base is higher priority result is -1
	 * 		if compared is higher priority result is 1
	 * 		else result is 0								*/
	static int8_t sleepCompare(TaskController const *base, TaskController const *compared) {
		if (base->m_wakeUpTimeStamp > compared->m_wakeUpTimeStamp)
			return 1;
		if (base->m_wakeUpTimeStamp < compared->m_wakeUpTimeStamp)
			return -1;
		if (base->m_wakeUpTimeStamp == compared->m_wakeUpTimeStamp) {
			if (base->m_priority > compared->m_priority)
				return -1;
			if (base->m_priority < compared->m_priority)
				return 1;
		}
		return 0;
	}

	/*Compare two tasks
	 * If base Task has higher priority (higher number) result is -1
	 * If priorities are equals result is 0
	 * If compared has higher priority result is 1*/
	static int8_t priorityCompare(TaskController const *base, TaskController const *compared) {
		Y_ASSERT((reinterpret_cast<std::size_t>(base) & 0xFFFF0000) ==0x20000000);
		Y_ASSERT((reinterpret_cast<std::size_t>(compared) & 0xFFFF0000) == 0x20000000);
		if (base->m_priority > compared->m_priority)
			return -1;
		if (base->m_priority < compared->m_priority)
			return 1;
		return 0;
	}

	void setReturnValue(uint32_t value) const{
		auto ctrl = *(m_stackPointer + 8);
		if ((ctrl & 0b100) == 0) // check bit #2 of control to know if floating point is active or not
			*(reinterpret_cast<volatile uint32_t*>(m_stackPointer + 10)) = value;
		else
			*(reinterpret_cast<volatile uint32_t*>(m_stackPointer + 26)) = value; //TODO Test
	}

	void setReturnValue(int16_t value) const{
		uint32_t ctrl = *(m_stackPointer + 1);
		if ((ctrl & 0b100) == 0) // check bit #2 of control to know if floating point is active or not
			*(reinterpret_cast<volatile int16_t*>(m_stackPointer + 10)) = value;
		else
			*(reinterpret_cast<volatile int16_t*>(m_stackPointer + 26)) = value; //TODO Test
	}

	inline void setStackPointer(uint32_t *stackPosition) {
		m_stackPointer = stackPosition;
#ifdef KDEBUG
			//m_stackUsage = m_stackSize - (stackPosition-m_stackOrigin);
#endif // KDEBUG
	}
};

template<uint32_t StackSize>
class Task {
public:
	constexpr Task<StackSize>() :
			m_ctrl(m_stack, StackSize) {
	}
	inline bool start(TaskController::TaskFunc function,const bool isPrivilegied, const uint32_t taskPriority, const uint32_t parameter = 0, const char *name = "") {
		return m_ctrl.start(function, isPrivilegied, taskPriority, parameter, name);
	}

	inline bool stop(){
		return m_ctrl.stop();
	}

private:
	uint32_t m_stack[StackSize]__attribute__((aligned(4)));
	TaskController m_ctrl;

};
} // namespace kernel
