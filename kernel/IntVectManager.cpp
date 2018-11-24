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

#include "IntVectManager.hpp"

using namespace kernel;

IntVectManager::IntVectManager()
{
	for (int i = 0; i < VECTOR_TABLE_SIZE; i++)
	{
		m_vectorTable[i] = (IrqHandler) &IntVectManager::defaultIsrHandler;
	}
}


IntVectManager::~IntVectManager()
{
	
}

void IntVectManager::defaultIsrHandler()
{
	asm("bkpt 0");
}

void IntVectManager::registerHandler(IRQn_Type irq, IrqHandler handler)
{
	m_vectorTable[((int16_t)irq) + 16] = handler;
}

void IntVectManager::registerHandler(uint16_t irq, IrqHandler handler)
{
	m_vectorTable[irq] = handler;
}

void IntVectManager::unregisterHandler(IRQn_Type irq)
{
	m_vectorTable[((int16_t)irq) + 16] = (IrqHandler) &IntVectManager::defaultIsrHandler;
}

uint32_t IntVectManager::tableBaseAddress()
{
	uint32_t baseAddress = (uint32_t)&m_vectorTable;
	return baseAddress;
}

IntVectManager::IrqHandler IntVectManager::getIsr(uint32_t isrNumber)
{
	return m_vectorTable[isrNumber];
}

uint8_t IntVectManager::s_numberOfSubBits;
uint8_t IntVectManager::s_numberOfPreEmptBits;


