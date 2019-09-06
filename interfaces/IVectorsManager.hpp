#pragma once

#include <cstdint>
#include "yggdrasil/kernel/Processor.hpp"

namespace core
{
	namespace interfaces
	{
		class IVectorManager
		{
			
		public:
			typedef IRQn_Type Irq;
			typedef void(*IrqHandler)(void);
			
		public:
			virtual void registerHandler(Irq irq, IrqHandler handler) = 0;
			virtual void unregisterHandler(Irq irq) = 0;
			virtual uint32_t tableBaseAddress() = 0;
			virtual IrqHandler getIsr(uint32_t isrNumber) = 0;
			virtual void irqPriority(Irq irq, uint8_t preEmptPriority, uint8_t subPriority) = 0;
			virtual void irqPriority(Irq irq, uint8_t globalPriority) = 0;
			virtual void subPriorityBits(uint8_t numberOfBits) = 0;
			virtual uint8_t subPriorityBits() = 0;
			virtual inline void enableIrq(Irq irq) = 0;
			virtual inline void disableIrq(Irq irq) = 0;
			virtual inline void clearIrq(Irq irq) = 0;
			virtual inline void lockInterruptsLowerThan(uint8_t Priority) = 0;
			virtual inline void unlockInterrupts() = 0;
			virtual inline void lockAllInterrupts() = 0;
			virtual inline void enableAllInterrupts() = 0;
			virtual inline bool isInstalled() = 0;
			virtual inline bool installVectorManager() = 0;
		private:
		};	//End class IVectorManager
	}	//End namespace interfaces
}	// End namespace core