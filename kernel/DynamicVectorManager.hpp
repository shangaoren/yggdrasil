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
#include "yggdrasil/systemview/segger/SEGGER_SYSVIEW.h"
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
		
		uint8_t m_numberOfSubBits;
		uint8_t m_numberOfPreEmptBits;
		bool m_tableInstalled;
		IrqHandler m_vectorTable[kVectorTableSize] __attribute__((aligned(kVectorTableAlignement)));  //must be aligned with next power of 2 of table size


		IntVectManager(uint8_t numberOfSubBits = 2)
			: m_numberOfSubBits(numberOfSubBits), m_numberOfPreEmptBits(kNumberOfEnabledPriorityBits - m_numberOfSubBits) , m_tableInstalled(false)
		{
			for (int i = 0; i < VECTOR_TABLE_SIZE; i++)
			{
				m_vectorTable[i] = reinterpret_cast<IrqHandler>(defaultIsrHandler);
			}
		}
		
		static void defaultIsrHandler()
		{
			asm("bkpt 0");
		}

		void registerHandler(IRQn_Type irq, IrqHandler handler)
		{
			m_vectorTable[static_cast<int16_t>(irq) + 16] = handler;
		}

		void registerHandler(uint16_t irq, IrqHandler handler)
		{
			m_vectorTable[irq] = handler;
		}

		void unregisterHandler(IRQn_Type irq)
		{
			m_vectorTable[static_cast<int16_t>(irq) + 16] =  reinterpret_cast<IrqHandler>(defaultIsrHandler);
		}

		uint32_t tableBaseAddress()
		{
			uint32_t baseAddress = (uint32_t)&m_vectorTable;
			return baseAddress;
		}

		IrqHandler getIsr(uint32_t isrNumber)
		{
			return m_vectorTable[isrNumber];
		}
		
		
		uint16_t getVectorTableSize()
		{
			return kVectorTableSize;
		}

		void irqPriority(IRQn_Type irq,uint8_t preEmptPriority, uint8_t subPriority)
		{
			uint8_t subMask = (0xFF >> (kNumberMaxOfInterruptBits - m_numberOfSubBits));
			uint8_t preEmptMask = (0xFF >> (kNumberMaxOfInterruptBits - m_numberOfPreEmptBits));

			if(subPriority > subMask)
				subPriority = subMask;
			if(preEmptPriority > preEmptMask)
				preEmptPriority = preEmptMask;

			uint8_t priority = static_cast<uint8_t>((subPriority)+
					(preEmptPriority << m_numberOfSubBits));
			NVIC_SetPriority(irq,priority);
		}

		void irqPriority(IRQn_Type irq, uint8_t globalPriority)
		{
			uint8_t priorityMask = (0xFF >> (kPiorityOffsetBits));
			if(globalPriority >priorityMask)
				globalPriority = priorityMask;

			NVIC_SetPriority(irq,globalPriority);
		}

		void subPriorityBits(uint8_t numberOfBits)
		{
			if(numberOfBits > kNumberOfEnabledPriorityBits)
				numberOfBits = kNumberOfEnabledPriorityBits;

			m_numberOfSubBits = numberOfBits;
			m_numberOfPreEmptBits = kNumberOfEnabledPriorityBits- m_numberOfSubBits;

			SCB->AIRCR = ((SCB->AIRCR & ~(SCB_AIRCR_VECTKEY_Msk | SCB_AIRCR_PRIGROUP_Msk))
					| ((kVectorKey << SCB_AIRCR_VECTKEY_Pos)| (m_numberOfSubBits << SCB_AIRCR_PRIGROUP_Pos)));

		}

		uint8_t subPriorityBits()
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
		
		static inline void lockInterruptsLowerThan(uint8_t Priority)
		{
			__set_BASEPRI(static_cast<uint32_t>(Priority));
		}
		
		static inline void unlockInterrupts()
		{
			__set_BASEPRI(0xFF);
		}
		
		static inline void lockAllInterrupts()
		{
			__set_PRIMASK(1);
		}
		
		static inline void enableAllInterrupts()
		{
			__set_PRIMASK(0);
		}
	};
}

