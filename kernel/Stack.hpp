#pragma once
#include <cstdint>
/*namespace kernel
{
class StackFrame
{
  public:
	constexpr StackFrame(uint32_t volatile *stackPointer) : m_stackPointer(stackPointer),
															Lr(stackPointer[0]),
															Control(stackPointer[1]),
															R4(stackPointer[2]),
															R5(stackPointer[3]),
															R6(stackPointer[4]),
															R7(stackPointer[5]),
															R8(stackPointer[6]),
															R9(stackPointer[7]),
															R10(stackPointer[8]),
															R11(stackPointer[9]),

															R0(stackPointer[10]),
															R1(stackPointer[11]),
															R2(stackPointer[12]),
															R3(stackPointer[13]),
															R12(stackPointer[14]),
															taskLr(stackPointer[15]),
															NextPc(stackPointer[16]),
															xPsr(stackPointer[17])
	{
	}

	void update()
	{
		Lr= m_stackPointer[0];
		Control = m_stackPointer[1];
		R4 = m_stackPointer[2];
		R5 = m_stackPointer[3];
		R6 = m_stackPointer[4];
		R7 = m_stackPointer[5];
		R8 = m_stackPointer[6];
		R9 = m_stackPointer[7];
		R10 = m_stackPointer[8];
		R11 = m_stackPointer[9];

		R0 = m_stackPointer[10];
		R1 = m_stackPointer[11];
		R2 = m_stackPointer[12];
		R3 = m_stackPointer[13];
		R12 = m_stackPointer[14];
		taskLr = m_stackPointer[15];
		NextPc = m_stackPointer[16];
		xPsr = m_stackPointer[17];
	}

  private:
	uint32_t volatile *m_stackPointer;
	uint32_t R0;
	uint32_t R1;
	uint32_t R2;
	uint32_t R3;
	uint32_t R12;
	uint32_t taskLr;
	uint32_t NextPc;
	uint32_t xPsr;

	uint32_t Lr;
	uint32_t Control;
	uint32_t R4;
	uint32_t R5;
	uint32_t R6;
	uint32_t R7;
	uint32_t R8;
	uint32_t R9;
	uint32_t R10;
	uint32_t R11;
	
	
};
}*/