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

#include "../framework/YList.hpp"
#include "../kernel/Waitable.hpp"

namespace kernel {
    class TaskController;

    class StartedListNode : public framework::YNode<TaskController> {};
    class ReadyListNode : public framework::YNode<TaskController> {};
    class WaitableListNode : public framework::YNode<TaskController> {};
    class EventListNode : public framework::YNode<TaskController> {};

    using StartedList = framework::YList<TaskController, StartedListNode>;
    using ReadyList = framework::YList<TaskController, ReadyListNode>;
    using WaitableList = framework::YList<TaskController, WaitableListNode>;
    using EventList = framework::YList<TaskController, EventListNode>;

    class TaskController : public StartedListNode, public ReadyListNode, public WaitableListNode, public EventListNode {
        friend class Scheduler;
        friend class Event;
        friend class Mutex;
        friend class Hooks;

    public:
        using TaskFunc = void (*)(uint32_t);
        using StartTaskStub = bool(&)(TaskController *);
        using StopTaskStub = bool(&)(TaskController *);

        bool start(TaskFunc function, bool isPrivilegied, uint32_t taskPriority, uint32_t parameter, const char *name);

        bool stop();

        [[nodiscard]] bool isStackCorrupted() const;

        static StartTaskStub &startTaskStub;
        static StopTaskStub &stopTaskStub;

        static void taskWrapper(TaskController &task, TaskFunc func, uint32_t parameter);

        static void taskFinished();

        enum class State : uint32_t {
            sleeping        = 0,
            active          = 1,
            waitingEvent    = 2,
            notStarted      = 3,
            ready           = 4,
            waitingMutex    = 5,
            nextActive      = 6
        };

        constexpr TaskController(uint32_t *stack, const uint32_t stackSize) : stackOrigin_(stack),
                                                                              stackSize_(stackSize) {
        }

    private:
        uint32_t volatile *stackPointer_ = nullptr;
        uint32_t *const stackOrigin_;
        const uint32_t stackSize_;

        std::atomic<uint32_t> wakeUpTimeStamp_ = 0;
        std::atomic<Waitable*> waitingFor_ = nullptr;
        uint32_t priority_ = 0;
        std::atomic<State> state_ = State::notStarted;
        const char *name_ = nullptr;

        [[nodiscard]] uint32_t wakeupTimestamp() const {
            return wakeUpTimeStamp_.load(std::memory_order_relaxed);
        }

        void wakeupTimestamp(const uint32_t value) {
            wakeUpTimeStamp_.store(value, std::memory_order_relaxed);
        }

        [[nodiscard]] Waitable* waitingFor() const {
            return waitingFor_.load(std::memory_order_relaxed);
        }

        void waitingFor(Waitable* const value) {
            waitingFor_.store(value, std::memory_order_relaxed);
        }

        [[nodiscard]] auto state() const {
            return state_.load();
        }

        void state(const State state) {
            state_ = state;
        }
        /*Compare two Task timestamps
         * if base task was running after compared result is 1
         * if compared was running before base result is -1
         * if timestamps are the same
         * 		if base is higher priority result is -1
         * 		if compared is higher priority result is 1
         * 		else result is 0								*/
        static int8_t sleepCompare(TaskController const *base, TaskController const *compared) {
            if (base->wakeupTimestamp() > compared->wakeupTimestamp())
                return 1;
            if (base->wakeupTimestamp() < compared->wakeupTimestamp())
                return -1;
            if (base->wakeupTimestamp() == compared->wakeupTimestamp()) {
                if (base->priority_ > compared->priority_)
                    return -1;
                if (base->priority_ < compared->priority_)
                    return 1;
            }
            return 0;
        }

        /*Compare two tasks
         * If base Task has higher priority (higher number) result is -1
         * If priorities are equals result is 0
         * If compared has a higher priority result is 1*/
        static int8_t priorityCompare(TaskController const *base, TaskController const *compared) {
            if (base->priority_ > compared->priority_)
                return -1;
            if (base->priority_ < compared->priority_)
                return 1;
            return 0;
        }

        void setReturnValue(uint32_t value) const {
            auto ctrl = *(stackPointer_ + 8);
            if ((ctrl & 0b100) == 0) // check bit #2 of control to know if floating point is active or not
                *(reinterpret_cast<volatile uint32_t *>(stackPointer_ + 10)) = value;
            else
                *(reinterpret_cast<volatile uint32_t *>(stackPointer_ + 26)) = value; //TODO Test
        }

        //TODO strange value + 1 vs + 8
        void setReturnValue(const int16_t value) const {
            uint32_t ctrl = *(stackPointer_ + 1);
            if ((ctrl & 0b100) == 0) // check bit #2 of control to know if floating point is active or not
                *(reinterpret_cast<volatile int16_t *>(stackPointer_ + 10)) = value;
            else
                *(reinterpret_cast<volatile int16_t *>(stackPointer_ + 26)) = value; //TODO Test
        }

        inline void setStackPointer(uint32_t *stackPosition) {
            stackPointer_ = stackPosition;
#ifdef KDEBUG
            //m_stackUsage = m_stackSize - (stackPosition-m_stackOrigin);
#endif // KDEBUG
        }
    };

    template<uint32_t StackSize>
    class Task {
        friend class Hooks;
    public:
        constexpr Task() : ctrl_(stack_, StackSize) {}
        Task(Task& ) = delete;
        Task &operator=(const Task&) = delete;
        Task(Task&&) = delete;
        Task &operator=(Task&&) = delete;

        bool start(TaskController::TaskFunc function, const bool isPrivilegied, const uint32_t taskPriority,
                          const uint32_t parameter = 0, const char *name = "") {
            return ctrl_.start(function, isPrivilegied, taskPriority, parameter, name);
        }

        bool stop() {
            return ctrl_.stop();
        }

    private:
        uint32_t stack_[StackSize]__attribute__((aligned(4)));
        TaskController ctrl_;
    };
} // namespace kernel
