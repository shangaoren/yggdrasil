#include "CortexM7.hpp"
#include "../YggdrasilConfig.hpp"
#include "../Yggdrasil.hpp"
#include <cstdint>

namespace core {

	void CortexM7::restoreTask(const volatile uint32_t *stackPointer)
	{
		asm volatile(
			"MOV R0,%0\n\t"			 // load stack pointer from task.stackPointer
			"LDMIA R0!,{R2-R11}\n\t" // restore registers R4 to R11
			"MOV LR,R2\n\t"			 // reload Link register
			"MSR CONTROL,R3\n\t"	 // reload CONTROL register
			"ISB\n\t"				 // Instruction synchronization Barrier is recommended after changing CONTROL
			"MSR PSP,R0\n\t"		 // reload process stack pointer with task's stack pointer
			"BX LR" ::"r"(stackPointer));
	}

	[[noreturn]] void CortexM7::idleFunc(uint32_t)
	{
		while (true)
		{
			__WFI();
		}
	}

	void __attribute__((naked)) CortexM7::contextSwitchHandler()
	{
		__asm volatile(
			"CPSID I\n\t"
			"DSB\n\t"
			"MRS R0, PSP\n\t" // store Process Stack pointer into R0
#ifdef FPU
		//"TST LR, #0x10\n\t"				   // test Bit 4 of Link return to know if floating point was used , if 0 save FP
		//"IT EQ\n\t"
		//"VSTMDBEQ R0!, {S16-S31}\n\t" // save floating point registers
#endif
			"MOV R2,LR\n\t"			 // store Link Register into R2
			"MRS R3,CONTROL\n\t"	 // store CONTROL register into R3
			"STMDB R0!,{R2-R11}\n\t" // store R2 to R11 memory pointed by R0 (stack), increment memory and rewrite the new adress to R0
			"bl %[changeTask]\n\t"	 // go to change task
			"LDMIA R0!,{R2-R11}\n\t" // restore registers R4 to R11 (R0 was loaded by change task with the new stackpointer
			"MOV LR,R2\n\t"			 // reload Link register
			"MSR CONTROL,R3\n\t"	 // reload CONTROL register
			"ISB\n\t"				 // Instruction synchronization Barrier is recommended after changing CONTROL
#ifdef FPU
		//"TST LR, #0x10\n\t"			  // test Bit 4 to know if FPU has to be restored, unstack if 0
		//"IT EQ\n\t"
		//"VLDMIAEQ R0!,{S16-S31}\n\t" //restore floating point registers
#endif
			"MSR PSP,R0\n\t" // reload process stack pointer with task's stack pointer
			"CPSIE I\n\t"	 // unlock Interrupts
			"ISB \n\t"
			"BX LR"
			: // no outputs
			: [changeTask] "g"(&kernel::Scheduler::taskSwitch)
			: "memory");
	}

	void CortexM7::systemTimerHandler()
	{
		kernel::Scheduler::systemTimerTick();
	}

	uint32_t CortexM7::getCoreFrequency() {
		return 64000000;
	}

	void __attribute__((naked)) CortexM7::supervisorCallHandler()
	{
		__asm volatile(
			"TST LR,#4\n\t"		// test bit 2 of EXC_RETURN to know if MSP or PSP
			"ITE EQ\n\t"		// was used for stacking
			"MRSEQ R1, MSP\n\t" // place msp or psp in R0 as parameter for SvcHandler
			"MRSNE R1, PSP\n\t"
			"LDR R0,[R1,#24]\n\t"
			"LDRB R0, [R0, #-2]\n\t"
			"PUSH {LR}\n\t" // stack LR to be able to return for exception
			"bl %[svc]\n\t"
			"POP {LR}\n\t"
			"BX LR"
			: // no outputs
			: [svc] "g"(&kernel::Scheduler::supervisorCall)
			:);
	}

	void CortexM7::hardFault()
	{
		breakpoint();
	}

	void CortexM7::nmi()
	{
		breakpoint();
	}

	void CortexM7::usageFault()
	{
		breakpoint();
	}

	void CortexM7::busFault()
	{
		breakpoint();
	}

	uint32_t CortexM7::getCurrentInterruptNumber()
	{
		return __get_IPSR();
	}

	void CortexM7::HardFaultAnalyzer(uint32_t *stackPointer)
	{
		breakpoint();
	}

    void CortexM7::breakpoint() {
		asm volatile("BKPT #0");
	}

    void CortexM7::fatalError() {
		asm volatile("BKPT #0");
	}
}