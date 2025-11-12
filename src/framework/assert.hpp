#pragma once
namespace kernel
{
	class Assertion {
	public:
		[[maybe_unused]] static void stop();
	};
#ifdef KDEBUG
	#define y_assert(cond) ((cond) ? (void)0U : kernel::Assertion::stop())
#else
	#define y_assert(cond) ((void)0)
#endif
}
