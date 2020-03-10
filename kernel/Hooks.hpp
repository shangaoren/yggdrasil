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
		
		
		static void onTaskStart(Task *task)
		{
#ifdef SYSVIEW
			kernel::SystemView::get().sendTaskInfo(&task);
			kernel::SystemView::get().onTaskCreate(&task);
			kernel::SystemView::get().onTaskStartReady(&task);
#endif
		}

		static void onTaskStartExec(Task *task)
		{
#ifdef SYSVIEW
			SystemView::get().onTaskStartExec(s_activeTask);
#endif
		}

		static void onTaskStopExec(Task *task)
		{
			
		}

		static void onTaskClose(Task *task)
		{
#ifdef SYSVIEW
			SystemView::get().onTaskTerminate(&task);
#endif
		}

		static void onTaskReady(Task *task)
		{
#ifdef SYSVIEW
			kernel::SystemView::get().onTaskStartReady(s_taskToStack);
#endif
		}

		static void onTaskSleep(Task *task, uint64_t time)
		{
#ifdef SYSVIEW
			SystemView::get().onTaskStopReady(s_taskToStack, static_cast<uint8_t>(changeTaskTrigger::enterSleep));
#endif
		}

		static void onTaskWaitEvent(Task *task, Event *event)
		{
			
		}

		static void onEventTrigger(Event *event)
		{
			
		}

		static void onEventTimeout(Event *event)
		{
			
		}
		
		/* Mutex*/
		static void onMutexLock(Mutex *mutex, Task* locker)
		{
			
		}

		static void onMutexRelease(Mutex *mutex)
		{
		}

		static void onMutexWait(Mutex *mutex,Task* waiter, uint32_t timeout)
		{
		}

		static void onMutexTimeout(Mutex *mutex, Task *task)
		{
			
		}
	};
} // namespace kernel
