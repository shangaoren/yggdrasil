#pragma once
#ifdef USE_DUMMY_VECTOR
#include <cstdint>
#include "../../../src/core/cortex_m/Irq.hpp"

namespace core {
template<uint16_t Size>
class DummyVector {
  public :
  	using IrqHandler = void(*)();

  private:
    static inline IrqHandler vectorTable_[Size] __attribute__((aligned(512))); //must be aligned with next power of 2 of table size
	static void defaultIsrHandler() {
		for (;;);
	}
	static constexpr uint16_t kNumberMaxOfInterruptBits = 8U;
	static constexpr uint16_t kNumberOfEnabledPriorityBits = 3;
	static constexpr uint16_t kPriorityOffsetBits = (kNumberMaxOfInterruptBits - kNumberOfEnabledPriorityBits);
	static constexpr uint16_t kVectorKey = 0x05FAU;

	static constexpr  uint8_t numberOfSubBits_ = 2;
	static constexpr uint8_t numberOfPreEmptBits_ = kNumberOfEnabledPriorityBits - numberOfSubBits_;
  public:

	static bool install() {
		for (int i = 0; i < Size; i++) {
			vectorTable_[i] = reinterpret_cast<IrqHandler>(defaultIsrHandler);
		}
		return true;
	}

  static void registerHandler(const Irq irq,
    IrqHandler handler, const char *name) {
      vectorTable_[static_cast<int16_t>(irq) + 16] = handler;
  }

  static void unregisterHandler(const Irq irq) {
		vectorTable_[static_cast<int16_t>(irq) + 16] =
				reinterpret_cast<IrqHandler>(defaultIsrHandler);
	}

	static uint32_t tableBaseAddress()
	{
		const auto baseAddress = reinterpret_cast<uint32_t>(&vectorTable_);
		return baseAddress;
	}

	static IrqHandler getIsr(uint32_t isrNumber)
	{
		return vectorTable_[isrNumber];
	}

	static void irqPriority(const Irq irq, uint8_t preEmptPriority, uint8_t subPriority)
        {
		const uint8_t subMask = (0xFF
				>> (kNumberMaxOfInterruptBits - numberOfSubBits_));
		const uint8_t preEmptMask = (0xFF
				>> (kNumberMaxOfInterruptBits - numberOfPreEmptBits_));

		if (subPriority > subMask)
			subPriority = subMask;
		if (preEmptPriority > preEmptMask)
			preEmptPriority = preEmptMask;

		uint8_t priority = static_cast<uint8_t>((subPriority)
				+ (preEmptPriority << numberOfSubBits_));
	}

	static void irqPriority(Irq irq, uint8_t globalPriority)
			{
		uint8_t priorityMask = (0xFF >> (kPriorityOffsetBits));
		if (globalPriority > priorityMask)
			globalPriority = priorityMask;
	}

	static uint8_t subPriorityBits()
	{
		return 0;
	}

	static void enableIrq(const Irq irq){
	}

	static void disableIrq(const Irq irq)
	{
	}

	static void clearIrq(const Irq irq)
	{
	}

	static uint8_t lockInterruptsHigherThan(const uint8_t priority){
		return 0;
	}

	static void unlockInterruptsHigherThan(uint8_t priorityToRestore){
	}

	static  void lockAllInterrupts()
	{
	}

	static void enableAllInterrupts()
	{
	}

	static bool isInstalled()
	{
		return true;
	}

	static inline bool installVectorManager()
	{
		return true;
	}
};
}
#endif