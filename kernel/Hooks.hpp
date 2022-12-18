#pragma once
#include "yggdrasil/kernel/Event.hpp"
#include "yggdrasil/kernel/Mutex.hpp"
#include "yggdrasil/kernel/Task.hpp"
#include "yggdrasil/interfaces/IWaitable.hpp"

namespace kernel
{
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
#ifdef SYSVIEW
			SystemView::get().onTaskStartExec(s_activeTask);
#endif
		}

		static void onTaskStopExec(TaskController *task)
		{
			
		}

		static void onTaskClose(TaskController *task)
		{
#ifdef SYSVIEW
			SystemView::get().onTaskTerminate(&task);
#endif
		}

		static void onTaskReady(TaskController *task)
		{
#ifdef SYSVIEW
			kernel::SystemView::get().onTaskStartReady(s_taskToStack);
#endif
		}

		static void onTaskSleep(TaskController *task, uint64_t time)
		{
#ifdef SYSVIEW
			SystemView::get().onTaskStopReady(s_taskToStack, static_cast<uint8_t>(changeTaskTrigger::enterSleep));
#endif
		}

		static void onTaskWaitEvent(TaskController *task, Event *event)
		{
			
		}

		static void onEventTrigger(Event *event)
		{
			
		}

		static void onEventTimeout(Event *event)
		{
			
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
