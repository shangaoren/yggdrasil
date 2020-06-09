#pragma once
namespace kernel
{
	static void stop()
	{
		asm volatile("bkpt #0");
	}
#ifdef KDEBUG
	#define Y_ASSERT(cond) ((cond) ? (void)0U : kernel::stop())
#else
	#define Y_ASSERT(cond) ((void)0)
#endif
}