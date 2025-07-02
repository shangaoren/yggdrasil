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
#include "../YggdrasilConfig.hpp"
#include "Hooks.hpp"

namespace kernel {

TaskController::StartTaskStub TaskController::startTaskStub = Core::SupervisorCallHelper<ServiceCall::SvcNumber::startTask, bool (TaskController*)>::call;
TaskController::StopTaskStub TaskController::stopTaskStub = Core::SupervisorCallHelper<ServiceCall::SvcNumber::stopTask, bool(TaskController*)>::call;

bool TaskController::start(TaskFunc function, bool isPrivilegied, uint32_t priority, uint32_t parameter = 0, const char *name = nullptr) {
	if (m_state != State::notStarted)
		return false;
    m_wakeUpTimeStamp = 0;
    m_waitingFor = nullptr;
    m_name = name;
	m_stackPointer = m_stackOrigin + m_stackSize - 18;
	m_stackOrigin[0] = 0xDEAD;
	m_stackOrigin[1] = 0xBEEF;
	m_stackOrigin[2] = 0xDEAD;
	m_stackOrigin[3] = 0xBEEF;
	m_stackOrigin[4] = 0xDEAD;
	m_stackOrigin[5] = 0xBEEF;
	m_stackOrigin[6] = 0xDEAD;
	m_stackOrigin[7] = 0xBEEF;

	//stacked by hardware
	m_stackPointer[17] = 0x01000000;							//initial xPSR
	m_stackPointer[16] = reinterpret_cast<uint32_t>(taskWrapper); 	//PC
	m_stackPointer[15] = reinterpret_cast<uint32_t>(taskFinished);  //LR
	m_stackPointer[14] = 4;											//R12
	m_stackPointer[13] = 3;											//R3
	m_stackPointer[12] = parameter;									//R2
	m_stackPointer[11] = reinterpret_cast<uint32_t>(function);				//R1
	m_stackPointer[10] = reinterpret_cast<uint32_t>(this);					//R0
	// software stacked registers
	m_stackPointer[9] = 11;						//R11
	m_stackPointer[8] = 10;						//R10
	m_stackPointer[7] = 9;						//R9
	m_stackPointer[6] = 8;						//R8
	m_stackPointer[5] = 7;						//R7
	m_stackPointer[4] = 6;						//R6
	m_stackPointer[3] = 5;						//R5
	m_stackPointer[2] = 4;						//R4
	m_stackPointer[1] = 0x2 | !isPrivilegied; //CONTROL, initial value, unprivileged, use PSP, no Floating Point
	m_stackPointer[0] = 0xFFFFFFFD;	//LR, return from exception, 8 Word Stack Length (no floating point), return in thread mode, use PSP
	m_priority = priority;
	startTaskStub(this);
	return true;
}


void TaskController::taskWrapper(TaskController &task, TaskFunc func, uint32_t parameter) {
	(*func)(parameter);
	stopTaskStub(&task);
	Core::breakpoint();
}

 void TaskController::taskFinished() {
	Core::breakpoint();
}

bool TaskController::stop(){
	if(this->m_state == State::notStarted || this->m_state == State::active)
		return false;
	stopTaskStub(this);
	return true;
}

bool TaskController::isStackCorrupted() const{
	if (m_stackOrigin[0] != 0xDEAD)
		return true;
	if (m_stackOrigin[1] != 0xBEEF)
		return true;
	if (m_stackOrigin[2] != 0xDEAD)
		return true;
	if (m_stackOrigin[3] != 0xBEEF)
		return true;
	if (m_stackOrigin[4] != 0xDEAD)
		return true;
	if (m_stackOrigin[5] != 0xBEEF)
		return true;
	if (m_stackOrigin[6] != 0xDEAD)
		return true;
	if (m_stackOrigin[7] != 0xBEEF)
		return true;
	return false;
}
}
