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
			virtual void registerHandler(Irq irq, IrqHandler handler, const char* name = nullptr) = 0;
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
			virtual inline uint8_t lockInterruptsHigherThan(uint8_t Priority) = 0;
			virtual inline void unlockInterruptsHigherThan(uint8_t Priority) = 0;
			virtual inline void lockAllInterrupts() = 0;
			virtual inline void enableAllInterrupts() = 0;
			virtual inline bool isInstalled() = 0;
			virtual inline bool installVectorManager() = 0;
		private:
		};	//End class IVectorManager
	}	//End namespace interfaces
}	// End namespace core