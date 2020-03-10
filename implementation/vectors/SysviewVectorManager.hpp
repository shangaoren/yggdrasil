/*MIT License

Copyright (c) 2019 Florian GERARD

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
#include <math.h>
#include "yggdrasil/kernel/Processor.hpp"
#include "yggdrasil/interfaces/IVectorsManager.hpp"
#include "DynamicVectorManager.hpp"
#include "yggdrasil/systemView/SystemView.hpp"


namespace implementation
{
	namespace vectors
	{
		using namespace core::interfaces;
		class SysviewVectorManager : public IVectorManager
		{
		  public:
			SysviewVectorManager(uint8_t numberOfSubBits = 2) : m_numberOfSubBits(numberOfSubBits), m_numberOfPreEmptBits(kNumberOfEnabledPriorityBits - m_numberOfSubBits), m_defaultHandler(&kernel::SystemView::SysviewIsrHandler)
			{
				for (int i = 0; i < VECTOR_TABLE_SIZE; i++)
				{
					m_stubTable[i] = reinterpret_cast<IrqHandler>(m_defaultHandler);
				}
				m_stubTable[PendSV_IRQn + 16] = &kernel::SystemView::pendSvHandler;
				m_stubTable[SVCall_IRQn + 16] = &kernel::SystemView::svcBootstrap;
			}

			virtual void __attribute__((optimize("O0"))) registerHandler(IVectorManager::Irq irq, IrqHandler handler, const char *name = nullptr) override final
			{
				// here pendsv, svc have no effect since they are overriden
				m_irqTable.registerHandler(irq ,handler);
				kernel::SystemView::get().registerInterrupt(name);
			}

			virtual void unregisterHandler(IVectorManager::Irq irq) override final
			{
				m_irqTable.unregisterHandler(irq);
			}

			virtual uint32_t tableBaseAddress() override final
			{
				uint32_t baseAddress = reinterpret_cast<uint32_t>(&m_stubTable);
				return baseAddress;
			}

			virtual IrqHandler getIsr(uint32_t isrNumber) override final
			{
				return m_irqTable.getIsr(isrNumber);
			}

			virtual void __attribute__((optimize("O0"))) irqPriority(IVectorManager::Irq irq, uint8_t preEmptPriority, uint8_t subPriority) override final
			{
				uint8_t subMask = (0xFF >> (kNumberMaxOfInterruptBits - m_numberOfSubBits));
				uint8_t preEmptMask = (0xFF >> (kNumberMaxOfInterruptBits - m_numberOfPreEmptBits));

				if (subPriority > subMask)
					subPriority = subMask;
				if (preEmptPriority > preEmptMask)
					preEmptPriority = preEmptMask;

				uint8_t priority = static_cast<uint8_t>((subPriority) +
														(preEmptPriority << m_numberOfSubBits));
				NVIC_SetPriority(irq, priority);
			}

			virtual void __attribute__((optimize("O0"))) irqPriority(IVectorManager::Irq irq, uint8_t globalPriority) override final
			{
				uint8_t priorityMask = (0xFF >> (kPiorityOffsetBits));
				if (globalPriority > priorityMask)
					globalPriority = priorityMask;

				NVIC_SetPriority(irq, globalPriority);
			}

			virtual void subPriorityBits(uint8_t numberOfBits) override final
			{
				if (numberOfBits > kNumberOfEnabledPriorityBits)
					numberOfBits = kNumberOfEnabledPriorityBits;

				m_numberOfSubBits = numberOfBits;
				m_numberOfPreEmptBits = kNumberOfEnabledPriorityBits - m_numberOfSubBits;

				SCB->AIRCR = ((SCB->AIRCR & ~(SCB_AIRCR_VECTKEY_Msk | SCB_AIRCR_PRIGROUP_Msk)) | ((kVectorKey << SCB_AIRCR_VECTKEY_Pos) | (m_numberOfSubBits << SCB_AIRCR_PRIGROUP_Pos)));
			}

			virtual uint8_t subPriorityBits() override final
			{
				return ((SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) >> (SCB_AIRCR_PRIGROUP_Pos + (kPiorityOffsetBits)));
			}

			virtual inline void __attribute__((optimize("O0"))) enableIrq(IVectorManager::Irq irq) override final
			{
				NVIC_EnableIRQ(irq);
			}

			virtual inline void disableIrq(IVectorManager::Irq irq) override final
			{
				NVIC_DisableIRQ(irq);
			}

			virtual inline void clearIrq(IVectorManager::Irq irq) override final
			{
				NVIC_ClearPendingIRQ(irq);
			}

			virtual inline void lockInterruptsHigherThan(uint8_t Priority) override final
			{
				__set_BASEPRI(static_cast<uint32_t>(Priority));
			}

			virtual inline void unlockInterrupts() override final
			{
				__set_BASEPRI(0x00);
			}

			virtual inline void __attribute__((optimize("O0"))) lockAllInterrupts() override final
			{
				asm volatile("CPSID I\n\t"
								"ISB");
			}

			virtual inline void __attribute__((optimize("O0"))) enableAllInterrupts() override final
			{
				asm volatile("CPSIE I\n\t"
								"ISB");
			}

			virtual inline bool isInstalled() override final
			{
				return SCB->VTOR == tableBaseAddress();
			}

			virtual inline bool installVectorManager() override final
			{
				SCB->VTOR = tableBaseAddress();
				return true;
			}

		  private:
			static constexpr uint16_t kVectorTableSize = VECTOR_TABLE_SIZE;
			static constexpr uint16_t kVectorTableAlignement = VECTOR_TABLE_ALIGNEMENT;
			static constexpr uint16_t kNumberMaxOfInterruptBits = 8U;
			static constexpr uint16_t kNumberOfEnabledPriorityBits = __NVIC_PRIO_BITS;
			static constexpr uint16_t kPiorityOffsetBits = (kNumberMaxOfInterruptBits - kNumberOfEnabledPriorityBits);
			static constexpr uint16_t kVectorKey = 0x05FAU;

			uint8_t m_numberOfSubBits;
			uint8_t m_numberOfPreEmptBits;
			IrqHandler m_stubTable[kVectorTableSize] __attribute__((aligned(kVectorTableAlignement))); //must be aligned with next power of 2 of table size
			DynamicVectorManager m_irqTable;
			IrqHandler m_defaultHandler;

			static void defaultIsrHandler()
			{
				uint8_t activeIsr = SCB->ICSR & 0x01FF;
				asm("bkpt 0");
				__get_IPSR();
			}



		}; // End class DynamicVectorManager
	}	  //End namespace vectors
} //End namespace implementation
