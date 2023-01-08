#pragma once
namespace kernel
{

#ifdef KDEBUG
static void stop()
{
	asm volatile("bkpt #0");
}
	#define Y_ASSERT(cond) ((cond) ? (void)0U : kernel::stop())
#else
	#define Y_ASSERT(cond) ((void)0)
#endif
}
