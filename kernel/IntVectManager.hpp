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
#include "Processor.hpp"
#include <math.h>

#ifdef SYSVIEW
#include "SEGGER_SYSVIEW.h"
#endif

namespace kernel
{
	class IntVectManager
	{
		friend class Api;
		friend class Scheduler;
	
	public:
	
		typedef void(*IrqHandler)(void);

	private:

		static constexpr uint16_t kVectorTableSize = VECTOR_TABLE_SIZE;
		static constexpr uint16_t kVectorTableAlignement = VECTOR_TABLE_ALIGNEMENT;
		static constexpr uint16_t kNumberMaxOfInterruptBits = 8U;
		static constexpr uint16_t kNumberOfEnabledPriorityBits = __NVIC_PRIO_BITS;
		static constexpr uint16_t kPiorityOffsetBits = (kNumberMaxOfInterruptBits - kNumberOfEnabledPriorityBits);
		static constexpr uint16_t kVectorKey = 0x05FAU;
		static uint8_t s_numberOfPreEmptBits;
		static uint8_t s_numberOfSubBits;

		IntVectManager();
		~IntVectManager();
		static void defaultIsrHandler();
		void registerHandler(IRQn_Type irq, IrqHandler handler);
		void registerHandler(uint16_t irq, IrqHandler handler);
		void unregisterHandler(IRQn_Type irq);
		uint32_t tableBaseAddress();
		IrqHandler getIsr(uint32_t isrNumber);
		
		IrqHandler m_vectorTable[kVectorTableSize] __attribute__((aligned(kVectorTableAlignement))); //must be aligned with next power of 2 of table size

		uint16_t getVectorTableSize()
		{
			return kVectorTableSize;
		}

		static void irqPriority(IRQn_Type irq,uint8_t preEmptPriority, uint8_t subPriority)
		{
			uint8_t subMask = (0xFF >> (kNumberMaxOfInterruptBits - s_numberOfSubBits));
			uint8_t preEmptMask = (0xFF >> (kNumberMaxOfInterruptBits - s_numberOfPreEmptBits));

			if(subPriority > subMask)
				subPriority = subMask;
			if(preEmptPriority > preEmptMask)
				preEmptPriority = preEmptMask;

			uint8_t priority = static_cast<uint8_t>((subPriority)+
					(preEmptPriority << s_numberOfSubBits));
			NVIC_SetPriority(irq,priority);
		}

		static void irqPriority(IRQn_Type irq, uint8_t globalPriority)
		{
			uint8_t priorityMask = (0xFF >> (kPiorityOffsetBits));
			if(globalPriority >priorityMask)
				globalPriority = priorityMask;

			NVIC_SetPriority(irq,globalPriority);
		}

		static void subPriorityBits(uint8_t numberOfBits)
		{
			if(numberOfBits > kNumberOfEnabledPriorityBits)
				numberOfBits = kNumberOfEnabledPriorityBits;

			s_numberOfSubBits = numberOfBits;
			s_numberOfPreEmptBits = kNumberOfEnabledPriorityBits-s_numberOfSubBits;

			SCB->AIRCR = ((SCB->AIRCR & ~(SCB_AIRCR_VECTKEY_Msk | SCB_AIRCR_PRIGROUP_Msk))
					| ((kVectorKey << SCB_AIRCR_VECTKEY_Pos)| (s_numberOfSubBits << SCB_AIRCR_PRIGROUP_Pos)));

		}

		static uint8_t subPriorityBits()
		{
			return ((SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) >> (SCB_AIRCR_PRIGROUP_Pos + (kPiorityOffsetBits)));
		}

		static inline void enableIrq(IRQn_Type irq)
		{
			NVIC_EnableIRQ(irq);
		}

		static inline void disableIrq(IRQn_Type irq)
		{
			NVIC_DisableIRQ(irq);
		}

		static inline void clearIrq(IRQn_Type irq)
		{
			NVIC_ClearPendingIRQ(irq);
		}
	};
}

