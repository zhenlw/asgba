/*
 * cpu_int_br.cpp
 *
 *  Created on: 2011-2-3
 *      Author: zlw
 */

FASTCALL uint32_t Op_BX(uint32_t ulOpCode)
{
	uint32_t rn = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
	TRACE_INSTR(rn, 0);
	if ( (rn & 0x01UL) == 0 ){
		g_regs[15] = rn & 0xFFFFFFFC;
	}
	else{
		g_regs[15] = rn & 0xFFFFFFFE;
		g_cpsr  |= CPSR_FLAG_MASK_T;
	}
	g_usTicksThisPiece += 2;
	return 1;	//prevent regular process from being taken
}

FASTCALL uint32_t Op_B(uint32_t ulOpCode)
{
	int32_t off = int32_t(ulOpCode << 8) >> 6;
	TRACE_INSTR(off, 0);
	g_regs[15] = g_regs[15] + off;
	g_usTicksThisPiece += 2;
	return 1;
}

FASTCALL uint32_t Op_BL(uint32_t ulOpCode)
{
	int32_t off = int32_t(ulOpCode << 8) >> 6;
	TRACE_INSTR(off, 0);
	g_regs[14] = g_regs[15] - 4;
	g_regs[15] = g_regs[15] + off;
	g_usTicksThisPiece += 2;
	return 1;
}

FASTCALL uint32_t Op_SWI(uint32_t ulOpCode)
{
	TRACE_INSTR(0, 0);
	RaiseExp(EXP_SWI, -4);
	return 1;	//prevent regular process from being taken
}
