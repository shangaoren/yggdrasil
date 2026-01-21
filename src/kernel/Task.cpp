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
	if (state_ != State::notStarted)
		return false;
    wakeUpTimeStamp_ = 0;
    waitingFor_ = nullptr;
    name_ = name;
	stackPointer_ = stackOrigin_ + stackSize_ - 18;
	stackOrigin_[0] = 0xDEAD;
	stackOrigin_[1] = 0xBEEF;
	stackOrigin_[2] = 0xDEAD;
	stackOrigin_[3] = 0xBEEF;
	stackOrigin_[4] = 0xDEAD;
	stackOrigin_[5] = 0xBEEF;
	stackOrigin_[6] = 0xDEAD;
	stackOrigin_[7] = 0xBEEF;

	//stacked by hardware
	stackPointer_[17] = 0x01000000;							//initial xPSR
	stackPointer_[16] = reinterpret_cast<uint32_t>(taskWrapper); 	//PC
	stackPointer_[15] = reinterpret_cast<uint32_t>(taskFinished);  //LR
	stackPointer_[14] = 4;											//R12
	stackPointer_[13] = 3;											//R3
	stackPointer_[12] = parameter;									//R2
	stackPointer_[11] = reinterpret_cast<uint32_t>(function);				//R1
	stackPointer_[10] = reinterpret_cast<uint32_t>(this);					//R0
	// software stacked registers
	stackPointer_[9] = 11;						//R11
	stackPointer_[8] = 10;						//R10
	stackPointer_[7] = 9;						//R9
	stackPointer_[6] = 8;						//R8
	stackPointer_[5] = 7;						//R7
	stackPointer_[4] = 6;						//R6
	stackPointer_[3] = 5;						//R5
	stackPointer_[2] = 4;						//R4
	stackPointer_[1] = 0x2 | !isPrivilegied; //CONTROL, initial value, unprivileged, use PSP, no Floating Point
	stackPointer_[0] = 0xFFFFFFFD;	//LR, return from exception, 8 Word Stack Length (no floating point), return in thread mode, use PSP
	priority_ = priority;
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
	if(this->state_ == State::notStarted || this->state_ == State::active)
		return false;
	stopTaskStub(this);
	return true;
}

bool TaskController::isStackCorrupted() const{
	if (stackOrigin_[0] != 0xDEAD)
		return true;
	if (stackOrigin_[1] != 0xBEEF)
		return true;
	if (stackOrigin_[2] != 0xDEAD)
		return true;
	if (stackOrigin_[3] != 0xBEEF)
		return true;
	if (stackOrigin_[4] != 0xDEAD)
		return true;
	if (stackOrigin_[5] != 0xBEEF)
		return true;
	if (stackOrigin_[6] != 0xDEAD)
		return true;
	if (stackOrigin_[7] != 0xBEEF)
		return true;
	return false;
}
}
