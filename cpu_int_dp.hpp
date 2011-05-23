/*
 * cpu_int_dp.cpp
 *
 *	the file is not to be compiled alone
 *
 *  Created on: 2011-2-2
 *      Author: zlw
 */

static uint32_t s_ulCarryOut;
//static uint32_t s_ulPcRestore;

uint32_t DataProcOpd2(uint32_t ulOpCode)
{
	uint32_t foo, samount;
	//s_ulPcRestore = g_pc;
	if ( (ulOpCode & (1UL << 25)) == 0 ){	//reg oprd 2
		uint32_t rmi = INT_BITS(uint32_t, ulOpCode, 0, 4);
		foo = g_regs[rmi];
		if ( (ulOpCode & (1UL << 4)) == 0 ){
			samount = INT_BITS(uint32_t, ulOpCode, 7, 5);
			switch ( INT_BITS(uint32_t, ulOpCode, 5, 2) ){
			case 0:
				if ( samount == 0 ){
					s_ulCarryOut = CPSR_FLAG_MASK_C & g_cpsr;
					PRINT_TRACE("  ----op2: r%d << %X = %X\n", rmi, samount, foo);
					return foo;
				}
				foo <<= ( samount - 1 );
				s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				foo <<= 1;
				PRINT_TRACE("  ----op2: r%d << %X = %X\n", rmi, samount, foo);
				return foo;
			case 1:
				if ( samount == 0 ) samount = 32;
				foo >>= samount - 1;
				s_ulCarryOut = ( foo  & 0x01 ) << 29;
				foo >>= 1;
				PRINT_TRACE("  ----op2: r%d >> %X = %X\n", rmi, samount, foo);
				return foo;
			case 2:
				if ( samount == 0 ) samount = 32;
				foo = uint32_t( int32_t(foo) >> ( samount - 1 ) );
				s_ulCarryOut = ( foo  & 0x01 ) << 29;
				foo = uint32_t(int32_t(foo) >> 1);
				PRINT_TRACE("  ----op2: signed r%d >> %X = %X\n", rmi, samount, foo);
				return foo;
			case 3:
				if ( samount == 0 ){
					s_ulCarryOut = ( foo  & 0x01 ) << 29;
					foo = ( foo >> 1 ) | ( ( g_cpsr & CPSR_FLAG_MASK_C ) << (31 - 29) );
					PRINT_TRACE("  ----op2: r%d ror with c flag (shift = 0) = %X\n", rmi, foo);
					return foo;
				}
				foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
				s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				PRINT_TRACE("  ----op2: r%d ror %X = %X\n", rmi, samount, foo);
			}
			return foo;	//case 3 only, to stop warning
		}
		else{
			g_usTicksThisPiece++;	//extra cycle needed for the 3rd reg
			g_pc += g_ulPcDelta;	//pc will be one more step ahead when if exception happens in this kind of instrs.
			uint32_t rsi = INT_BITS(uint32_t, ulOpCode, 8, 4);
			samount = g_regs[rsi] & 0x00FF;
			if ( samount == 0 ){
				s_ulCarryOut = CPSR_FLAG_MASK_C & g_cpsr;
				PRINT_TRACE("  ----op2: r%d >> r%d low byte (%X) = %X\n", rmi, rsi, samount, foo);
				return foo;
			}
			switch ( INT_BITS(uint32_t, ulOpCode, 5, 2) ){
			case 0:
				foo <<= ( samount - 1 );
				s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				foo <<= 1;
				PRINT_TRACE("  ----op2: r%d << r%d low byte (%X) = %X\n", rmi, rsi, samount, foo);
				break;
			case 1:
				foo >>= samount - 1;
				s_ulCarryOut = ( foo  & 0x01 ) << 29;
				foo >>= 1;
				PRINT_TRACE("  ----op2: r%d >> r%d low byte (%X) = %X\n", rmi, rsi, samount, foo);
				break;
			case 2:
				foo = uint32_t( int32_t(foo) >> ( samount - 1 ) );
				s_ulCarryOut = ( foo  & 0x01 ) << 29;
				foo = uint32_t( int32_t(foo) >> 1 );
				PRINT_TRACE("  ----op2: signed r%d >> r%d low byte (%X) = %X\n", rmi, rsi, samount, foo);
				break;
			case 3:
				samount %= 32;
				foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
				s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				PRINT_TRACE("  ----op2: signed r%d ror r%d low byte (%X) = %X\n", rmi, rsi, samount, foo);
			}
			return foo;
		}
	}
	else{	//immd oprd2
		foo = ulOpCode & 0x000000FF;
		samount = INT_BITS(uint32_t, ulOpCode, 8, 4) << 1;
		if ( samount == 0 ){
			s_ulCarryOut = CPSR_FLAG_MASK_C & g_cpsr;	//do we need carry out on immd oprd2?
			PRINT_TRACE("  ----op2: imm %X\n", foo);
			return foo;
		}
		foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
		s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );	//do we need carry out on immd oprd2?
		PRINT_TRACE("  ----op2: imm ror %X = %X\n", samount, foo);
		return foo;
	}
}

FASTCALL uint32_t Op_AND(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);	//also pc is decided here
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t res = g_regs[rni] & ulOpd2;
	
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		PRINT_TRACE("  ----no S, r%d (%X) = r%d (%X) & %X\n", rdi, res, rni, g_regs[rni], ulOpd2);
		g_regs[rdi] = res;
		if ( rdi == 15 ) return 1;
	}
	else{
		PRINT_TRACE("  ----with S, r%d (%X) = r%d (%X) & %X\n", rdi, res, rni, g_regs[rni], ulOpd2);
		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	}
	//g_pc = s_ulPcRestore;	//restore the pc on regular process
	return 0;
}

FASTCALL uint32_t Op_EOR(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] ^ ulOpd2;
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = res;
		if ( rdi == 15 ) return 1;
	}
	else{
		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	}
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_TST(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	//S must be set. or assumed to be set even when not?
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t res = g_regs[rni] & ulOpd2;

	PRINT_TRACE("  ----no rd, %X = r%d (%X) & %X\n", res, rni, g_regs[rni], ulOpd2);

	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_TEQ(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] ^ ulOpd2;
	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_ORR(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] | ulOpd2;
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = res;
		if ( rdi == 15 ) return 1;
	}
	else{
		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	}
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_MOV(uint32_t ulOpCode)
{
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);

	TRACE_INSTR(ulOpCode, rdi);

	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);

	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = ulOpd2;
		if ( rdi == 15 ) return 1;
	}
	else{
		uint32_t res = ulOpd2;
		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	}
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_BIC(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);

	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);

	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = g_regs[rni] & (~ulOpd2);
		PRINT_TRACE("  ----no S, r%d (%X) = r%d (%X) & ~%X\n", rdi, g_regs[rdi], rni, g_regs[rni], ulOpd2);
		if ( rdi == 15 ) return 1;
	}
	else{
		uint32_t res = g_regs[rni] & (~ulOpd2);
		PRINT_TRACE("  ----with S, r%d (%X) = r%d (%X) & ~%X\n", rdi, res, rni, g_regs[rni], ulOpd2);
		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	}
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_MVN(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t res = ~ulOpd2;
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = res;
		if ( rdi == 15 ) return 1;
	}
	else{
		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	}
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_ADD(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = g_regs[rni] + ulOpd2;
		PRINT_TRACE("  ----  no S, r%d = r%d (%X) + %X\n", rdi, rni, g_regs[rni], ulOpd2);
		if ( rdi == 15 ) return 1;
	}
	else{
		uint32_t op1 = g_regs[rni];
		uint32_t res = op1 + ulOpd2;
		PRINT_TRACE("  ----with S, r%d = r%d (%X) + %X\n", rdi, rni, op1, ulOpd2);
		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		uint32_t bit33 = (ulOpd2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit of this var
		if ( op1 > res ) {g_cpsr |= CPSR_FLAG_MASK_C;  bit33 += 0x80000000; }
		else g_cpsr &= ~CPSR_FLAG_MASK_C;
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( bit33 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	}
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_CMN(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	//S must be set
	uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
	uint32_t op2 = ulOpd2;
	uint32_t res = op1 + op2;
	//rd ignored
	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	op2 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit of op2
	if ( op1 > res ) {g_cpsr |= CPSR_FLAG_MASK_C;  op2 += 0x80000000; }
	else g_cpsr &= ~CPSR_FLAG_MASK_C;
	g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( op2 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_ADC(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] + ulOpd2 + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 29 );
		if ( rdi == 15 ) return 1;
	}
	else{
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = ulOpd2;
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 29 );

		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		if ( op2 > res ) cflag = CPSR_FLAG_MASK_C;
		g_cpsr = g_cpsr & (~CPSR_FLAG_MASK_C) | cflag;
		bit33 += cflag << (31 - 29);
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( bit33 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	}
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_SUB(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = g_regs[rni] - ulOpd2;
		PRINT_TRACE("  ----no S, r%d (%X) = r%d (%X) - %X\n", rdi, g_regs[rdi], rni, g_regs[rni], ulOpd2);
		if ( rdi == 15 ) return 1;
	}
	else{
		uint32_t op1 = g_regs[rni];
		uint32_t op2 = ~ulOpd2;
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + 1;

		PRINT_TRACE("  ----with S, r%d (%X) = r%d (%X) - %X\n", rdi, res, rni, g_regs[rni], ulOpd2);

		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		if ( op2 > res ) cflag = CPSR_FLAG_MASK_C;
		g_cpsr = g_cpsr & (~CPSR_FLAG_MASK_C) | cflag;
		bit33 += cflag << (31 - 29);
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( bit33 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	}
	//g_pc = s_ulPcRestore;
	return 0;
}

FASTCALL uint32_t Op_SBC(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] - ulOpd2 + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 29 ) - 1;
		if ( rdi == 15 ) return 1;
	}
	else{
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = ~ulOpd2;
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 29 );

		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		if ( op2 > res ) cflag = CPSR_FLAG_MASK_C;
		g_cpsr = g_cpsr & (~CPSR_FLAG_MASK_C) | cflag;
		bit33 += cflag << (31 - 29);
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( bit33 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	}
	//g_pc = s_ulPcRestore;

	return 0;
}

FASTCALL uint32_t Op_CMP(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);

	uint32_t op1 = g_regs[rni];
	uint32_t op2 = ~ulOpd2;
	uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
	op2 += op1;
	uint32_t cflag = 0;
	if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
	uint32_t res = op2 + 1;

	PRINT_TRACE("  ----no rd, %X = r%d (%X) - %X\n", res, rni, g_regs[rni], ulOpd2);

	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	if ( op2 > res ) cflag = CPSR_FLAG_MASK_C;
	g_cpsr = g_cpsr & (~CPSR_FLAG_MASK_C) | cflag;
	bit33 += cflag << (31 - 29);
	g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( bit33 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	//g_pc = s_ulPcRestore;

	return 0;
}

FASTCALL uint32_t Op_RSB(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = ulOpd2 - g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		if ( rdi == 15 ) return 1;
	}
	else{
		uint32_t op2 = ~g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op1 = ulOpd2;
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + 1;

		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		if ( op2 > res ) cflag = CPSR_FLAG_MASK_C;
		g_cpsr = g_cpsr & (~CPSR_FLAG_MASK_C) | cflag;
		bit33 += cflag << (31 - 29);
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( bit33 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	}
	//g_pc = s_ulPcRestore;

	return 0;
}

FASTCALL uint32_t Op_RSC(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	if ( (ulOpCode & ( 0x1UL << 20 )) == 0 ){
		g_regs[rdi] = ulOpd2 - g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 29 ) -1;
		if ( rdi == 15 ) return 1;
	}
	else{
		uint32_t op2 = ~g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op1 = ulOpd2;
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 29 );

		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		if ( op2 > res ) cflag = CPSR_FLAG_MASK_C;
		g_cpsr = g_cpsr & (~CPSR_FLAG_MASK_C) | cflag;
		bit33 += cflag << (31 - 29);
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( bit33 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	}
	//g_pc = s_ulPcRestore;

	return 0;
}
