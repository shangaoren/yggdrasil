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


#include "Kernel.hpp"

namespace kernel
{

		bool Task::start()
		{
			if (Scheduler::s_interruptInstalled == false)
				Scheduler::installSystemInterrupts();
			return startTaskStub(this);
		}

		bool Task::stop()
		{
			__BKPT(0);
			return true;
		}

		void Task::taskFinished()
		{
			__BKPT(0);
		}

		bool __attribute__((naked)) Task::startTaskStub(Task* task)
		{
			asm volatile("PUSH {LR}");
			svc(ServiceCall::SvcNumber::startTask);
			asm volatile(
				"POP {LR}\n\t"
				"BX LR");
		}
}
