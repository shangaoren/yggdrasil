#pragma once
#include "Scheduler.hpp"
#include "yggdrasil/src/kernel/Waitable.hpp"

namespace kernel
{
	class Mutex;
	class Event;
	class TaskController;

	class Hooks
	{
	  public:
		static void onKernelStart()
		{
#ifdef SYSVIEW
			kernel::SystemView::get().start();
#endif
		}
		
		
		static void onTaskStart(TaskController *task)
		{
#ifdef SYSVIEW
			kernel::SystemView::get().sendTaskInfo(&task);
			kernel::SystemView::get().onTaskCreate(&task);
			kernel::SystemView::get().onTaskStartReady(&task);
#endif
		}

		static void onTaskStartExec(TaskController *task)
		{
			if (Scheduler::ready.empty() && core::Core::idleTask.m_ctrl.state() == TaskController::State::ready) {
				core::Core::breakpoint();
			}
#ifdef SYSVIEW
			SystemView::get().onTaskStartExec(s_activeTask);
#endif
		}

		static void onTaskStopExec(TaskController *task)
		{
			if (Scheduler::ready.empty() && core::Core::idleTask.m_ctrl.state() == TaskController::State::ready) {
				core::Core::breakpoint();
			}
			
		}

		static void onTaskClose(TaskController *task)
		{
#ifdef SYSVIEW
			SystemView::get().onTaskTerminate(&task);
#endif
		}

		static void onTaskReady(TaskController *task)
		{
			if (Scheduler::ready.empty() && core::Core::idleTask.m_ctrl.state() == TaskController::State::ready) {
				core::Core::breakpoint();
			}
#ifdef SYSVIEW
			kernel::SystemView::get().onTaskStartReady(s_taskToStack);
#endif
		}

		static void onTaskSleep(TaskController *task, uint64_t time)
		{
			if (Scheduler::ready.empty() && core::Core::idleTask.m_ctrl.state() == TaskController::State::ready) {
				core::Core::breakpoint();
			}
#ifdef SYSVIEW
			SystemView::get().onTaskStopReady(s_taskToStack, static_cast<uint8_t>(changeTaskTrigger::enterSleep));
#endif
		}

		static void onTaskWaitEvent(TaskController *task, Event *event)
		{
			if (Scheduler::ready.empty() && core::Core::idleTask.m_ctrl.state() == TaskController::State::ready) {
				core::Core::breakpoint();
			}
		}

		static void onEventTrigger(Event *event)
		{
			if (Scheduler::ready.empty() && core::Core::idleTask.m_ctrl.state() == TaskController::State::ready) {
				core::Core::breakpoint();
			}
		}

		static void onEventTimeout(Event *event)
		{
			if (Scheduler::ready.empty() && core::Core::idleTask.m_ctrl.state() == TaskController::State::ready) {
				core::Core::breakpoint();
			}
		}
		
		/* Mutex*/
		static void onMutexLock(Mutex *mutex, TaskController* locker)
		{
			
		}

		static void onMutexRelease(Mutex *mutex)
		{
		}

		static void onMutexWait(Mutex *mutex,TaskController* waiter, uint32_t timeout)
		{
		}

		static void onMutexTimeout(Mutex *mutex, TaskController *task)
		{
			
		}
	};
} // namespace kernel
