/*
 * cpu_int_dp.cpp
 *
 *	the file is not to be compiled alone
 *
 *  Created on: 2011-2-2
 *      Author: zlw
 */

FASTCALL uint32_t Op_AND(uint32_t ulOpCode)
{
	//g_ulTicksThisPiece++; should be done in the common routine
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] =
				g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & DataProcOpd2(ulOpCode);
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & DataProcOpd2(ulOpCode);
		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | ulCarryOut;	//must there be a carry out with opd2?
	}
	return 0;
}

FASTCALL uint32_t Op_EOR(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] =
				g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] ^ DataProcOpd2(ulOpCode);
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] ^ DataProcOpd2(ulOpCode);
		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | ulCarryOut;	//must there be a carry out with opd2?
	}
	return 0;
}

FASTCALL uint32_t Op_TST(uint32_t ulOpCode)
{
	//S must be set. or assumed to be set even when not?
	uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & DataProcOpd2(ulOpCode);
	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | ulCarryOut;	//must there be a carry out with opd2?
	return 0;
}

FASTCALL uint32_t Op_TEQ(uint32_t ulOpCode)
{
	uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] ^ DataProcOpd2(ulOpCode);
	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | ulCarryOut;	//must there be a carry out with opd2?
	return 0;
}

FASTCALL uint32_t Op_ORR(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] =
				g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] | DataProcOpd2(ulOpCode);
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] | DataProcOpd2(ulOpCode);
		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | ulCarryOut;	//must there be a carry out with opd2?
	}
	return 0;
}

FASTCALL uint32_t Op_MOV(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = DataProcOpd2(ulOpCode);
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = DataProcOpd2(ulOpCode);
		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | ulCarryOut;	//must there be a carry out with opd2?
	}
	return 0;
}

FASTCALL uint32_t Op_BIC(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] =
				g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & (~DataProcOpd2(ulOpCode));
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] & (~DataProcOpd2(ulOpCode));
		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | ulCarryOut;	//must there be a carry out with opd2?
	}
	return 0;
}

FASTCALL uint32_t Op_MVN(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = ~DataProcOpd2(ulOpCode);
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t res = ~DataProcOpd2(ulOpCode);
		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
		}
		g_regs[rdi] = res;
		if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
		else g_cpsr &= ~CPSR_FLAG_MASK_Z;
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		g_cpsr = ( g_cpsr & ~CPSR_FLAG_MASK_C ) | ulCarryOut;	//must there be a carry out with opd2?
	}
	return 0;
}

FASTCALL uint32_t Op_ADD(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] + DataProcOpd2(ulOpCode);
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = DataProcOpd2(ulOpCode);
		uint32_t res = op1 + op2;
		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
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
	return 0;
}

FASTCALL uint32_t Op_CMN(uint32_t ulOpCode)
{
	//S must be set
	uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
	uint32_t op2 = DataProcOpd2(ulOpCode);
	uint32_t res = op1 + op2;
	//rd ignored
	if ( res == 0 ) g_cpsr |= CPSR_FLAG_MASK_Z;
	else g_cpsr &= ~CPSR_FLAG_MASK_Z;
	g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
	op2 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit of op2
	if ( op1 > res ) {g_cpsr |= CPSR_FLAG_MASK_C;  op2 += 0x80000000; }
	else g_cpsr &= ~CPSR_FLAG_MASK_C;
	g_cpsr = g_cpsr & ( ~CPSR_FLAG_MASK_V ) | (( ( op2 ^ res ) & 0x80000000 ) >> ( 31 - 28 ));	//oVerflow on different 33bit & 32bit
	return 0;
}

FASTCALL uint32_t Op_ADC(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] + DataProcOpd2(ulOpCode) + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 28 );
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = DataProcOpd2(ulOpCode);
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + ( (g_cpsr | CPSR_FLAG_MASK_C) >> 28 );

		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
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
	return 0;
}

FASTCALL uint32_t Op_SUB(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] - DataProcOpd2(ulOpCode);
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = ~DataProcOpd2(ulOpCode);
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + 1;

		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
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
	return 0;
}

FASTCALL uint32_t Op_SBC(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] - DataProcOpd2(ulOpCode) + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 28 ) - 1;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op2 = ~DataProcOpd2(ulOpCode);
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + ( (g_cpsr | CPSR_FLAG_MASK_C) >> 28 );

		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
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
	return 0;
}

FASTCALL uint32_t Op_CMP(uint32_t ulOpCode)
{
	uint32_t op1 = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
	uint32_t op2 = ~DataProcOpd2(ulOpCode);
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
	return 0;
}

FASTCALL uint32_t Op_RSB(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = DataProcOpd2(ulOpCode) - g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op2 = ~g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op1 = DataProcOpd2(ulOpCode);
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + 1;

		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
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
	return 0;
}

FASTCALL uint32_t Op_RSC(uint32_t ulOpCode)
{
	if ( ulOpCode & ( 0x1UL << 20 ) == 0 ){
		g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = DataProcOpd2(ulOpCode) - g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] + ( (g_cpsr & CPSR_FLAG_MASK_C) >> 28 ) -1;
	}
	else{
		uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
		uint32_t op2 = ~g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];
		uint32_t op1 = DataProcOpd2(ulOpCode);
		uint32_t bit33 = (op2 & 0x80000000) + (op1 & 0x80000000);	//sign extended "33rd" bit at the 32nd bit
		op2 += op1;
		uint32_t cflag = 0;
		if ( op1 > op2 ) cflag = CPSR_FLAG_MASK_C;
		uint32_t res = op2 + ( (g_cpsr | CPSR_FLAG_MASK_C) >> 28 );

		if ( rdi == 15 ){
			BackFromExp(res);
			return 0;
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
	return 0;
}
