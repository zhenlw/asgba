/*
 * cpu_int_dp.cpp
 *
 *	the file is not to be compiled alone
 *
 *  Created on: 2011-2-2
 *      Author: zlw
 */

static uint32_t s_ulCarryOut;
static uint32_t s_ulPcDelta;

uint32_t DataProcOpd2(uint32_t ulOpCode)
{
	uint32_t foo, samount;
	s_ulPcDelta = 0;
	if ( ulOpCode & (1UL << 25) == 0 ){	//reg oprd 2
		if ( ulOpCode & (1UL << 4) == 0 ){
			foo = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
			samount = INT_BITS(uint32_t, ulOpCode, 7, 5);
			switch ( INT_BITS(uint32_t, ulOpCode, 5, 2) ){
			case 0b00:
				if ( samount == 0 ){
					s_ulCarryOut = CPSR_FLAG_MASK_C & g_cpsr;
					return foo;
				}
				foo <<= ( samount - 1 );
				s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				return foo << 1;
			case 0b01:
				if ( samount == 0 ) samount = 32;
				foo >>= samount - 1;
				s_ulCarryOut = ( foo  & 0x01 ) << 29;
				return foo >> 1;
			case 0b10:
				if ( samount == 0 ) samount = 32;
				foo = uint32_t( int32_t(foo) >> ( samount - 1 ) );
				s_ulCarryOut = ( foo  & 0x01 ) << 29;
				return uint32_t( int32_t(foo) >> 1 );
			case 0b11:
				if ( samount == 0 ){
					s_ulCarryOut = ( foo  & 0x01 ) << 29;
					return ( foo >> 1 ) | ( ( g_cpsr & CPSR_FLAG_MASK_C ) << (31 - 29) );
				}
				foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
				s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				return foo;
			}
		}
		else{
			g_ulTicksThisPiece++;	//extra cycle needed for the 3rd reg
			g_regs[15] += 4;
			s_ulPcDelta = 4;
			foo = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
			samount = g_regs[INT_BITS(uint32_t, ulOpCode, 8, 4)] & 0xFF;
			if ( samount == 0 ){
				s_ulCarryOut = CPSR_FLAG_MASK_C & g_cpsr;
				return foo;
			}
			switch ( INT_BITS(uint32_t, ulOpCode, 5, 2) ){
			case 0b00:
				foo <<= ( samount - 1 );
				s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				return foo << 1;
			case 0b01:
				foo >>= samount - 1;
				s_ulCarryOut = ( foo  & 0x01 ) << 29;
				return foo >> 1;
			case 0b10:
				foo = uint32_t( int32_t(foo) >> ( samount - 1 ) );
				s_ulCarryOut = ( foo  & 0x01 ) << 29;
				return uint32_t( int32_t(foo) >> 1 );
			case 0b11:
				samount %= 32;
				foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
				s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				return foo;
			}
		}
	}
	else{	//immd oprd2
		foo = ulOpCode & 0x000000FF;
		samount = INT_BITS(uint32_t, ulOpCode, 8, 4) << 1;
		if ( samount == 0 ){
			s_ulCarryOut = CPSR_FLAG_MASK_C & g_cpsr;	//do we need carry out on immd oprd2?
			return foo;
		}
		foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
		s_ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );	//do we need carry out on immd oprd2?
		return foo;
	}
}

FASTCALL uint32_t Op_AND(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);	//also pc is decided here
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] =
				g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & ulOpd2;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & ulOpd2;
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
	g_regs[15] -= s_ulPcDelta;	//restore the pc on regular process
	return 0;
}

FASTCALL uint32_t Op_EOR(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] =
				g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] ^ ulOpd2;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] ^ ulOpd2;
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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_TST(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	//S must be set. or assumed to be set even when not?
	uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & ulOpd2;
	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_TEQ(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] ^ ulOpd2;
	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | s_ulCarryOut;	//must there be a carry out with opd2?
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_ORR(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] =
				g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] | ulOpd2;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] | ulOpd2;
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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_MOV(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = ulOpd2;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_BIC(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] =
				g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & (~ulOpd2);
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & (~ulOpd2);
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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_MVN(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = ~ulOpd2;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = ~ulOpd2;
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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_ADD(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] + ulOpd2;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = ulOpd2;
		uint32_t res = op1 + op2;
		if ( rdi == 15 ){
			BackFromExp(res);
			return 1;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		op2 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit of op2
		if ( op1 > res ) {g_cpsr |= CPSR_FLAG_MASK_C;  op2 += 0x80000000; }
		else g_cpsr &= ~CPSR_FLAG_MASK_C;
		g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( op2 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	}
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_CMN(uint32_t ulOpCode)
{
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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_ADC(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] + ulOpd2 + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 28 );
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = ulOpd2;
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + ( (g_cpsr | CPSR_FLAG_MASK_C) >> 28 );

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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_SUB(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] - ulOpd2;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = ~ulOpd2;
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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_SBC(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] - ulOpd2 + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 28 ) - 1;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = ~ulOpd2;
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + ( (g_cpsr | CPSR_FLAG_MASK_C) >> 28 );

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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_CMP(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
	uint32_t op2 = ~ulOpd2;
	uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
	op2 += op1;
	uint32_t cflag = 0;
	if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
	uint32_t res = op2 + 1;
	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	if ( op2 > res ) cflag = CPSR_FLAG_MASK_C;
	g_cpsr = g_cpsr & (~CPSR_FLAG_MASK_C) | cflag;
	bit33 += cflag << (31 - 29);
	g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( bit33 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_RSB(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = ulOpd2 - g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}

FASTCALL uint32_t Op_RSC(uint32_t ulOpCode)
{
	uint32_t ulOpd2 = DataProcOpd2(ulOpCode);
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = ulOpd2 - g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 28 ) -1;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op2 = ~g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op1 = ulOpd2;
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + ( (g_cpsr | CPSR_FLAG_MASK_C) >> 28 );

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
	g_regs[15] -= s_ulPcDelta;
	return 0;
}
