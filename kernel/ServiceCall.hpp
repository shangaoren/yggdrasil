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
#include "IntVectManager.hpp"
#include "Processor.hpp"

#define svc(code) asm volatile("SVC %[immediate]"::[immediate] "I" (code))

namespace kernel
{
	class ServiceCall
	{
		friend class Api;
		
	public:
		
		enum class SvcNumber : uint8_t
		{
			startFirstTask,
			registerIrq,
			unregisterIrq,
			setGlobalPriority,
			setPriority,
			clearIrq,
			enableIrq,
			disableIrq,
			startTask,
			stopTask,
			sleepTask,
			signalEvent,
			waitEvent,
			enterCriticalSection,
			exitCriticalSection,
		};
	};
}
