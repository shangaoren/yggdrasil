#pragma once
#include <cstdint>
#include "../framework/assert.hpp"
#include "core/vendor/st/stm32f303x8.h"

namespace core {
  class Irq
  {
  public:
    constexpr explicit Irq(const int16_t number) : interruptNumber_(number){}

    constexpr explicit operator int16_t() const
    {
      return interruptNumber_;
    }

  private:
    const int16_t interruptNumber_;
  };
template<uint16_t Size>
class DynamicVector {
  public :
  	using IrqHandler = void(*)();

  private:
    static inline IrqHandler vectorTable_[Size] __attribute__((aligned(512))); //must be aligned with next power of 2 of table size
	static void defaultIsrHandler() {
		uint8_t activeIsr = SCB->ICSR & 0x01FF;
		asm("bkpt 0");
		__get_IPSR();
	}
	static constexpr uint16_t kNumberMaxOfInterruptBits = 8U;
	static constexpr uint16_t kNumberOfEnabledPriorityBits = __NVIC_PRIO_BITS;
	static constexpr uint16_t kPriorityOffsetBits = (kNumberMaxOfInterruptBits - kNumberOfEnabledPriorityBits);
	static constexpr uint16_t kVectorKey = 0x05FAU;

	static constexpr  uint8_t numberOfSubBits_ = 2;
	static constexpr uint8_t numberOfPreEmptBits_ = kNumberOfEnabledPriorityBits - numberOfSubBits_;
  public:

	static bool install() {
		for (int i = 0; i < Size; i++) {
			vectorTable_[i] = reinterpret_cast<IrqHandler>(defaultIsrHandler);
		}
		SCB->VTOR = tableBaseAddress();
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
		NVIC_SetPriority(static_cast<IRQn_Type>(static_cast<int16_t>(irq)),
				priority);
	}

	static void irqPriority(Irq irq, uint8_t globalPriority)
			{
		uint8_t priorityMask = (0xFF >> (kPriorityOffsetBits));
		if (globalPriority > priorityMask)
			globalPriority = priorityMask;

		NVIC_SetPriority(static_cast<IRQn_Type>(static_cast<int16_t>(irq)),
				globalPriority);
	}

	static uint8_t subPriorityBits()
	{
		return ((SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk)
				>> (SCB_AIRCR_PRIGROUP_Pos + (kPriorityOffsetBits)));
	}

	static void enableIrq(const Irq irq)
			{
		NVIC_EnableIRQ(static_cast<IRQn_Type>(static_cast<int16_t>(irq)));
	}

	static void disableIrq(const Irq irq)
	{
		NVIC_DisableIRQ(static_cast<IRQn_Type>(static_cast<int16_t>(irq)));
	}

	static void clearIrq(const Irq irq)
	{
		NVIC_ClearPendingIRQ(static_cast<IRQn_Type>(static_cast<int16_t>(irq)));
	}

	static uint8_t lockInterruptsHigherThan(const uint8_t priority)
			{
		uint8_t result = __get_BASEPRI() >> kPriorityOffsetBits;
		y_assert(priority != 0);
		if ((result > priority) | (result == 0))
			__set_BASEPRI(priority << kPriorityOffsetBits);
		return result;
	}

	static void unlockInterruptsHigherThan(uint8_t priorityToRestore)
			{
		y_assert(priorityToRestore != 0xF0);
		auto result = __get_BASEPRI() >> kPriorityOffsetBits;
		if ((result < priorityToRestore) | (priorityToRestore == 0))
			__set_BASEPRI(priorityToRestore << kPriorityOffsetBits);
		__ISB();
	}

	static  void lockAllInterrupts()
	{
		__set_PRIMASK(1);
	}

	static void enableAllInterrupts()
	{
		__set_PRIMASK(0);
	}

	static bool isInstalled()
	{
		return SCB->VTOR == tableBaseAddress();
	}

	static inline bool installVectorManager()
	{
		SCB->VTOR = tableBaseAddress();
		return true;
	}
};
}