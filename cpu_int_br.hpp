/*
 * cpu_int_br.cpp
 *
 *  Created on: 2011-2-3
 *      Author: zlw
 */

FASTCALL uint32_t Op_BX(uint32_t ulOpCode)
{
	uint32_t rn = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
	TRACE_INSTR(ulOpCode, rn);
	if ( (rn & 0x01UL) == 0 ){
		PRINT_TRACE("  ----to ARM: %X\n", rn);
		g_regs[15] = rn & 0xFFFFFFFC;
		g_cpsr  &= ~CPSR_FLAG_MASK_T;
	}
	else{
		PRINT_TRACE("  ----to THUMB: %X\n", rn);
		g_regs[15] = rn & 0xFFFFFFFE;
		g_cpsr  |= CPSR_FLAG_MASK_T;
	}
	g_usTicksThisPiece += 2;
	return 1;	//prevent regular process from being taken
}

FASTCALL uint32_t Op_B(uint32_t ulOpCode)
{
	int32_t off = int32_t(ulOpCode << 8) >> ( 6 + 2 - ( g_ulPcDelta >> 1 ) );	//compatible with thumb
	TRACE_INSTR(ulOpCode, off);
	g_regs[15] = g_regs[15] + off;
	g_usTicksThisPiece += 2;
	return 1;
}

FASTCALL uint32_t Op_BL(uint32_t ulOpCode)
{
	if ( g_ulPcDelta == 2 ){
		TRACE_INSTR(0, ulOpCode);
		if ( INT_BITS(uint32_t, ulOpCode, 11, 1) == 0 ){
			g_regs[14] = g_pc + ( int32_t(ulOpCode << 21) >> 9 );	//left shift 12 with sign ext
			//should this pc be used? or the next
			return 0;
		}
		uint32_t ul = g_pc - 2;
		g_pc = g_regs[14] + ( INT_BITS(uint32_t, ulOpCode, 0, 11) << 1 );
		g_regs[14] = ul | 0x01UL;
		g_usTicksThisPiece += 2;
		return 1;
	}
	int32_t off = int32_t(ulOpCode << 8) >> 6;
	TRACE_INSTR(ulOpCode, off);
	g_regs[14] = g_regs[15] - 4;
	g_regs[15] = g_regs[15] + off;
	g_usTicksThisPiece += 2;
	return 1;
}

FASTCALL uint32_t Op_SWI(uint32_t)
{
	TRACE_INSTR(0, 0);
	RaiseExp(EXP_SWI, g_pc - g_ulPcDelta);
	return 1;	//prevent regular process from being taken
}
