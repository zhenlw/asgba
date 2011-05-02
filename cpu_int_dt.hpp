/*
 * cpu_int_dt.cpp
 *
 *  Created on: 2011-2-4
 *      Author: zlw
 */

static uint32_t s_ulCarryOut1;

static inline uint32_t AddrOffsetBW(uint32_t ulOpCode)
{
	uint32_t foo, samount;

	if ( (ulOpCode & (1UL << 25)) == 0 ){
		s_ulCarryOut1 = CPSR_FLAG_MASK_C & g_cpsr;	//c flag untouched
		return ulOpCode & 0x0FFF;
	}

	//if ( ulOpCode & (1UL << 4) == 0 )	//must this be satisfied?
	foo = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
	samount = INT_BITS(uint32_t, ulOpCode, 7, 5);
	switch ( INT_BITS(uint32_t, ulOpCode, 5, 2) ){
	case 0:
		if ( samount == 0 ){
			s_ulCarryOut1 = CPSR_FLAG_MASK_C & g_cpsr;
			return foo;
		}
		foo <<= ( samount - 1 );
		s_ulCarryOut1 = ( foo & 0x80000000 ) >> ( 31 - 29 );
		return foo << 1;
	case 1:
		if ( samount == 0 ) samount = 32;
		foo >>= samount - 1;
		s_ulCarryOut1 = ( foo  & 0x01 ) << 29;
		return foo >> 1;
	case 2:
		if ( samount == 0 ) samount = 32;
		foo = uint32_t( int32_t(foo) >> ( samount - 1 ) );
		s_ulCarryOut1 = ( foo  & 0x01 ) << 29;
		return uint32_t( int32_t(foo) >> 1 );
	case 3:
		if ( samount == 0 ){
			s_ulCarryOut1 = ( foo  & 0x01 ) << 29;
			return ( foo >> 1 ) | ( ( g_cpsr & CPSR_FLAG_MASK_C ) << (31 - 29) );
		}
		foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
		s_ulCarryOut1 = ( foo & 0x80000000 ) >> ( 31 - 29 );
	}
	return foo; //case 3 only, to suppress warning
}

FASTCALL uint32_t Op_LDRB(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t ulOff = ( (ulOpCode & (1UL << 23)) != 0 )? AddrOffsetBW(ulOpCode): 0UL - AddrOffsetBW(ulOpCode);
	uint32_t ulAddr = g_regs[rni] + ulOff;
	g_cpsr = g_cpsr & ~CPSR_FLAG_MASK_C | s_ulCarryOut1;	//the carry flag probably should not be set, or should not be set here.

	g_usTicksThisPiece += 2;

	if ( (ulOpCode & (1UL << 24)) != 0 ){	//pre-exe write-back
		if ( (ulOpCode & (1UL << 21)) != 0 ){	//write back, usually this is done even an exception happen, but how about early abort?
			g_regs[rni] =  ulAddr;	//rn cannot be r15/PC, but if it is, no problem to set it as well, since no behavior is defined
		}
		g_regs[15] += g_ulPcDelta;	//when exception happens, the pc is 1 more word ahead
		g_regs[rdi] = uint32_t(phym_read8(ulAddr));	//exception handling here? just let it through to save some coding for now
		PRINT_TRACE("  ----Pre, r%d = [r%d + %X] (%X)\n", rdi, rni, ulOff, g_regs[rdi]);
	}
	else{
		uint32_t ulAddr1 = g_regs[rni];	//the 1st exe cycle thing, but actually this may not be necessary since rni cannot be pc
		g_regs[15] += g_ulPcDelta;
		g_regs[rdi] = uint32_t(phym_read8(ulAddr1));	//non-privilege transfer on 'W'? even the real thing does that, apps probably won't cross the line even if the emu doesn't act the same
		g_regs[rni] = ulAddr;	//no write-back to PC; & for post-alternating, it is a mandatory.
		PRINT_TRACE("  ----post, r%d = [r%d] (%X), off = %X\n", rdi, rni, g_regs[rdi], ulOff);
	}
	if ( rdi == 15 ){
		g_usTicksThisPiece += 2;
		return 1;
	}
	//g_regs[15] -= 4;
	return 0;
}

FASTCALL uint32_t Op_LDRW(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t rn = ( rni == 15 )? g_regs[15] & 0xFFFFFFFC: g_regs[rni];	//for thumb ldr pc relative
	uint32_t ulOff = ( (ulOpCode & (1UL << 23)) != 0 )? AddrOffsetBW(ulOpCode): 0UL - AddrOffsetBW(ulOpCode);
	uint32_t ulAddr = rn + ulOff;
	g_cpsr = g_cpsr & ~CPSR_FLAG_MASK_C | s_ulCarryOut1;	//the carry flag probably should not be set, or should not be set here.

	g_usTicksThisPiece += 2;

	if ( (ulOpCode & (1UL << 24)) != 0 ){	//pre-exe offsetting
		if ( (ulOpCode & (1UL << 21)) != 0 ){	//write back, usually this is done even an exception happen, but how about early abort?
			g_regs[rni] =  ulAddr;	//rn cannot be r15/PC, but if it is, no problem to set it as well, since no behavior is defined
		}
		g_regs[15] += g_ulPcDelta;	//when exception happens, the pc is 1 more word ahead
		uint32_t foo = phym_read32(ulAddr);
		g_regs[rdi] = (foo >> ((ulAddr & 3UL) * 8)) | (foo << (32 - (ulAddr & 3UL) * 8));
		PRINT_TRACE("  ----Pre, r%d = [r%d + %X] (%X)\n", rdi, rni, ulOff, g_regs[rdi]);
	}
	else{
		uint32_t ulAddr1 = rn;	//the 1st exe cycle thing, but actually this may not be necessary since rni cannot be pc
		g_regs[15] += g_ulPcDelta;
		uint32_t foo = phym_read32(ulAddr1);
		g_regs[rdi] = (foo >> ((ulAddr1 & 3UL) * 8)) | (foo << (32 - (ulAddr1 & 3UL) * 8));
		g_regs[rni] = ulAddr;	//no write-back to PC; & for post-alternating, it is a mandatory.
		PRINT_TRACE("  ----post, r%d = [r%d] (%X), off = %X\n", rdi, rni, g_regs[rdi], ulOff);
	}
	if ( rdi == 15 ){
		g_usTicksThisPiece += 2;
		return 1;
	}
	//g_regs[15] -= 4;
	return 0;
}

FASTCALL uint32_t Op_STRB(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t rsi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t ulAddr = ( (ulOpCode & (1UL << 23)) != 0 )? g_regs[rni] + AddrOffsetBW(ulOpCode): g_regs[rni] - AddrOffsetBW(ulOpCode);
	g_cpsr = g_cpsr & ~CPSR_FLAG_MASK_C | s_ulCarryOut1;	//the carry flag probably should not be set, or should not be set here.

	g_usTicksThisPiece ++;

	if ( (ulOpCode & (1UL << 24)) != 0 ){	//pre-exe write-back
		if ( (ulOpCode & (1UL << 21)) != 0 ){	//write back, usually this is done even an exception happen, but how about early abort?
			g_regs[rni] =  ulAddr;	//rn cannot be r15/PC, but if it is, no problem to set it as well, since no behavior is defined
		}

		g_regs[15] += g_ulPcDelta;	//the source reg is read at the 2nd exe cycle, where pc is one more word ahead
		phym_write8(ulAddr, uint8_t(g_regs[rsi]));
	}
	else{
		uint32_t ulAddr1 = g_regs[rni];	//the 1st exe cycle thing, but actually this may not be necessary since rni cannot be pc
		g_regs[15] += g_ulPcDelta;
		phym_write8(ulAddr1, uint8_t(g_regs[rsi]));
		g_regs[rni] = ulAddr;	//no write-back to PC; & for post-alternating, it is a mandatory.
	}
	//g_regs[15] -= 4;

	return 0;
}

FASTCALL uint32_t Op_STRW(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t rsi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t ulAddr = ( (ulOpCode & (1UL << 23)) != 0 )? g_regs[rni] + AddrOffsetBW(ulOpCode): g_regs[rni] - AddrOffsetBW(ulOpCode);
	g_cpsr = g_cpsr & ~CPSR_FLAG_MASK_C | s_ulCarryOut1;	//the carry flag probably should not be set, or should not be set here.

	g_usTicksThisPiece ++;

	if ( (ulOpCode & (1UL << 24)) != 0 ){	//pre-exe write-back
		if ( (ulOpCode & (1UL << 21)) != 0 ){	//write back, usually this is done even an exception happen, but how about early abort?
			g_regs[rni] =  ulAddr;	//rn cannot be r15/PC, but if it is, no problem to set it as well, since no behavior is defined
		}

		g_regs[15] += g_ulPcDelta;	//the source reg is read at the 2nd exe cycle, where pc is one more word ahead
		phym_write32(ulAddr & 0xFFFFFFFC/*not sure this is right*/, g_regs[rsi]);
		PRINT_TRACE("  ----Pre, [%X] = r%d (%X)\n", ulAddr, rsi, g_regs[rsi]);
	}
	else{
		uint32_t ulAddr1 = g_regs[rni];	//the 1st exe cycle thing, but actually this may not be necessary since rni cannot be pc
		g_regs[15] += g_ulPcDelta;
		phym_write32(ulAddr1 & 0xFFFFFFFC, g_regs[rsi]);
		PRINT_TRACE("  ----Post, [%X] = r%d (%X)\n", ulAddr1, rsi, g_regs[rsi]);
		g_regs[rni] = ulAddr;	//no write-back to PC; & for post-alternating, it is a mandatory.
	}
	//g_regs[15] -= 4;

	return 0;
}

//half word & signed
static inline uint32_t AddrOffsetHS(uint32_t ulOpCode)
{
	if ( (ulOpCode & (1UL << 22)) != 0 ){
		return ((ulOpCode & 0x0F00UL) >> 4) | (ulOpCode & 0x0FUL);
	}
	return g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
}

FASTCALL uint32_t Op_LDRH(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t ulOff = ( (ulOpCode & (1UL << 23)) != 0 )? AddrOffsetHS(ulOpCode): 0UL - AddrOffsetHS(ulOpCode);
	uint32_t ulAddr = g_regs[rni] + ulOff;

	g_usTicksThisPiece += 2;

	if ( (ulOpCode & (1UL << 24)) != 0 ){	//pre-exe write-back
		if ( (ulOpCode & (1UL << 21)) != 0 ){	//write back, usually this is done even an exception happen, but how about early abort?
			g_regs[rni] =  ulAddr;	//rn cannot be r15/PC, but if it is, no problem to set it as well, since no behavior is defined
		}
		g_regs[15] += g_ulPcDelta;	//when exception happens, the pc is 1 more word ahead
		g_regs[rdi] = uint32_t(phym_read16(ulAddr));	//exception handling here? just let it through to save some coding for now
		PRINT_TRACE("  ----Pre, r%d = [r%d + %X] = (%X)\n", rdi, rni, ulOff, g_regs[rdi]);
	}
	else{
		uint32_t ulAddr1 = g_regs[rni];	//the 1st exe cycle thing, but actually this may not be necessary since rni cannot be pc
		g_regs[15] += g_ulPcDelta;
		g_regs[rdi] = uint32_t(phym_read16(ulAddr1));	//non-privilege transfer on 'W'? even the real thing does that, apps probably won't cross the line even if the emu doesn't act the same
		g_regs[rni] = ulAddr;	//no write-back to PC; & for post-alternating, it is a mandatory.
		PRINT_TRACE("  ----Post, r%d = [r%d] = (%X), offset = %X\n", rdi, rni, g_regs[rdi], ulOff);
	}
	if ( rdi == 15 ){
		g_usTicksThisPiece += 2;
		return 1;
	}
	//g_regs[15] -= 4;
	return 0;
}

FASTCALL uint32_t Op_LDRSH(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t ulAddr = ( (ulOpCode & (1UL << 23)) != 0 )? g_regs[rni] + AddrOffsetHS(ulOpCode): g_regs[rni] - AddrOffsetHS(ulOpCode);

	g_usTicksThisPiece += 2;

	if ( (ulOpCode & (1UL << 24)) != 0 ){	//pre-exe write-back
		if ( (ulOpCode & (1UL << 21)) != 0 ){	//write back, usually this is done even an exception happen, but how about early abort?
			g_regs[rni] =  ulAddr;	//rn cannot be r15/PC, but if it is, no problem to set it as well, since no behavior is defined
		}
		g_regs[15] += g_ulPcDelta;	//when exception happens, the pc is 1 more word ahead
		g_regs[rdi] = uint32_t(int32_t(int16_t(phym_read16(ulAddr))));
	}
	else{
		uint32_t ulAddr1 = g_regs[rni];	//the 1st exe cycle thing, but actually this may not be necessary since rni cannot be pc
		g_regs[15] += g_ulPcDelta;
		g_regs[rdi] = uint32_t(int32_t(int16_t(phym_read16(ulAddr1))));
		g_regs[rni] = ulAddr;	//no write-back to PC; & for post-alternating, it is a mandatory.
	}
	if ( rdi == 15 ){
		g_usTicksThisPiece += 2;
		return 1;
	}
	//g_regs[15] -= 4;
	return 0;
}

FASTCALL uint32_t Op_LDRSB(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t ulAddr = ( (ulOpCode & (1UL << 23)) != 0 )? g_regs[rni] + AddrOffsetHS(ulOpCode): g_regs[rni] - AddrOffsetHS(ulOpCode);

	g_usTicksThisPiece += 2;

	if ( (ulOpCode & (1UL << 24)) != 0 ){	//pre-exe write-back
		if ( (ulOpCode & (1UL << 21)) != 0 ){	//write back, usually this is done even an exception happen, but how about early abort?
			g_regs[rni] =  ulAddr;	//rn cannot be r15/PC, but if it is, no problem to set it as well, since no behavior is defined
		}
		g_regs[15] += g_ulPcDelta;	//when exception happens, the pc is 1 more word ahead
		g_regs[rdi] = uint32_t(int32_t(int8_t(phym_read8(ulAddr))));
	}
	else{
		uint32_t ulAddr1 = g_regs[rni];	//the 1st exe cycle thing, but actually this may not be necessary since rni cannot be pc
		g_regs[15] += g_ulPcDelta;
		g_regs[rdi] = uint32_t(int32_t(int8_t(phym_read8(ulAddr1))));
		g_regs[rni] = ulAddr;	//no write-back to PC; & for post-alternating, it is a mandatory.
	}
	if ( rdi == 15 ){
		g_usTicksThisPiece += 2;
		return 1;
	}
	//g_regs[15] -= 4;
	return 0;
}

FASTCALL uint32_t Op_STRH(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t rsi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t ulAddr = ( (ulOpCode & (1UL << 23)) != 0 )? g_regs[rni] + AddrOffsetHS(ulOpCode): g_regs[rni] - AddrOffsetHS(ulOpCode);
	g_cpsr = g_cpsr & ~CPSR_FLAG_MASK_C | s_ulCarryOut1;	//the carry flag probably should not be set, or should not be set here.

	g_usTicksThisPiece ++;

	if ( (ulOpCode & (1UL << 24)) != 0 ){	//pre-exe write-back
		if ( (ulOpCode & (1UL << 21)) != 0 ){	//write back, usually this is done even an exception happen, but how about early abort?
			g_regs[rni] =  ulAddr;	//rn cannot be r15/PC, but if it is, no problem to set it as well, since no behavior is defined
		}

		g_regs[15] += g_ulPcDelta;	//the source reg is read at the 2nd exe cycle, where pc is one more word ahead
		phym_write16(ulAddr & 0xFFFFFFFE/*not sure this is right*/, uint16_t(g_regs[rsi]));
	}
	else{
		uint32_t ulAddr1 = g_regs[rni];	//the 1st exe cycle thing, but actually this may not be necessary since rni cannot be pc
		g_regs[15] += g_ulPcDelta;
		phym_write16(ulAddr1 & 0xFFFFFFFE, uint16_t(g_regs[rsi]));
		g_regs[rni] = ulAddr;	//no write-back to PC; & for post-alternating, it is a mandatory.
	}
	//g_regs[15] -= 4;

	return 0;
}

FASTCALL uint32_t Op_LDM(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t ulAddr = g_regs[rni] & 0xFFFFFFFC;

	//calc the addresses, etc...
	uint32_t ulCount = 0;
	bool bR15 = false;
	uint32_t ulTargetRn;
	if ( (ulOpCode & ( 1UL << 15 )) != 0 ){
		ulCount++;
		bR15 = true;
	}
	for ( int i = 14; i >= 0; i--){
		if ( (ulOpCode & ( 1UL << i )) != 0 ) ulCount++;
	}
	if ( (ulOpCode & (1UL << 23)) != 0 ){	//inc
		ulTargetRn = ulAddr + ulCount * 4;
		if ( (ulOpCode & (1UL << 24)) != 0 )	//pre
			ulAddr += 4;
	}
	else{
		ulAddr -= ulCount * 4;
		ulTargetRn = ulAddr;
		if ( (ulOpCode & (1UL << 24)) == 0 )	//post
			ulAddr += 4;
	}
	if ( (ulOpCode & (1UL << 21)) == 0 ) ulTargetRn = g_regs[rni];	//not sure if this is correct on abort

	g_usTicksThisPiece++;
	g_regs[15] += g_ulPcDelta;	//when exception happens, the pc is 1 more word ahead?

	g_regs[rni] = ulTargetRn;	//rn is always in the view of current mode

	if ( (ulOpCode & (1UL << 22)) != 0 ){	//S
		if ( bR15 ){
			try{
				for ( int i = 0; i < 15; i++ ){
					if ( (ulOpCode & ( 1UL << i )) != 0 ){
						g_usTicksThisPiece++;
						g_regs[i] = phym_read32(ulAddr);
						PRINT_TRACE("  ----ld with S r%d = [%X] %X\n", i, ulAddr, g_regs[i]);
						ulAddr += 4;
					}
				}
				g_usTicksThisPiece++;
				uint32_t pc = phym_read32(ulAddr);
				PRINT_TRACE("  ----ld with S pc = [%X] %X\n", ulAddr, pc);
				BackFromExp(pc);
				return 1;
			}
			catch (uint32_t){
				g_regs[rni] = ulTargetRn;	//not sure restore is right on W==0, but otherwise not reasonable if rn is written
				RaiseExp(EXP_ABT, g_pc - 4);
				return 1;
			}
		}
		else{	//load to user mode registers
			SwitchRegs(g_cpsr & 0x0FUL, MODE_USR);
			try{
				for ( int i = 0; i < 15; i++ ){
					if ( (ulOpCode & ( 1UL << i )) != 0 ){
						g_usTicksThisPiece++;
						g_regs[i] = phym_read32(ulAddr);
						PRINT_TRACE("  ----ld USERMODE r%d = [%X] %X\n", i, ulAddr, g_regs[i]);
						ulAddr += 4;
					}
				}
			}
			catch (uint32_t){
				SwitchRegs(MODE_USR, g_cpsr & 0x0FUL);
				g_regs[rni] = ulTargetRn;	//rn still can be written with user view, should rn be restored once?
				RaiseExp(EXP_ABT, g_pc - 4);
				return 1;
			}
			SwitchRegs(MODE_USR, g_cpsr & 0x0FUL);
		}
	}
	else{
		try{
			for ( int i = 0; i < 15; i++ ){
				if ( (ulOpCode & ( 1UL << i )) != 0 ){
					g_usTicksThisPiece++;
					g_regs[i] = phym_read32(ulAddr);
					PRINT_TRACE("  ----ld no S r%d = [%X] %X\n", i, ulAddr, g_regs[i]);
					ulAddr += 4;
				}
			}
			if ( bR15 ){
				g_usTicksThisPiece++;
				g_regs[15] = phym_read32(ulAddr);
				PRINT_TRACE("  ----ld no S pc = [%X] %X\n", ulAddr, g_regs[15]);
				return 1;
			}
		}
		catch (uint32_t){
			g_regs[rni] = ulTargetRn;
			RaiseExp(EXP_ABT, g_pc - 4);
			return 1;
		}
	}
	//g_regs[15] -= 4;
	return 0;
}

FASTCALL uint32_t Op_STM(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t rni = INT_BITS(uint32_t, ulOpCode, 16, 4);
	uint32_t ulAddr = g_regs[rni] & 0xFFFFFFFC;

	//calc the addresses, etc...
	uint32_t ulCount = 0;
	uint32_t ulFirst = 0;
	uint32_t ulTargetRn;
	for ( int i = 15; i >= 0; i--){
		if ( (ulOpCode & ( 1UL << i )) != 0 ){
			ulCount++;
			ulFirst = (uint32_t)i;
		}
	}
	if ( (ulOpCode & (1UL << 23)) != 0 ){	//inc
		ulTargetRn = ulAddr + ulCount * 4;
		if ( (ulOpCode & (1UL << 24)) != 0 )	//pre
			ulAddr += 4;
	}
	else{
		ulAddr -= ulCount * 4;
		ulTargetRn = ulAddr;
		if ( (ulOpCode & (1UL << 24)) == 0 )	//post
			ulAddr += 4;
	}
	if ( (ulOpCode & (1UL << 21)) == 0 ) ulTargetRn = g_regs[rni];	//not sure if this is correct on abort

	g_usTicksThisPiece++;
	g_regs[15] += g_ulPcDelta;	//when read, the pc is 1 more word ahead

	if ( (ulOpCode & (1UL << 22)) != 0 ){	//S
		//load to user mode registers, W must be 0
		SwitchRegs(g_cpsr & 0x0FUL, MODE_USR);
		try{
			for ( int i = 0; i < 16; i++ ){
				if ( (ulOpCode & ( 1UL << i )) != 0 ){
					g_usTicksThisPiece++;
					phym_write32(ulAddr, g_regs[i]);
					PRINT_TRACE("  ----st with S [%X] = r%d (%X)\n", ulAddr, i, g_regs[i]);
					ulAddr += 4;
				}
			}
		}
		catch (uint32_t){
			SwitchRegs(MODE_USR, g_cpsr & 0x0FUL);
			RaiseExp(EXP_ABT, g_pc - 4);
			return 1;
		}
		SwitchRegs(MODE_USR, g_cpsr & 0x0FUL);
	}
	else{
		try{
			for ( int i = 0; i < 15; i++ ){
				if ( (ulOpCode & ( 1UL << i )) != 0 ){
					g_usTicksThisPiece++;
					phym_write32(ulAddr, g_regs[i]);
					PRINT_TRACE("  ----st no S [%X] = r%d (%X)\n", ulAddr, i, g_regs[i]);
					ulAddr += 4;
					g_regs[rni] = ulTargetRn;	//to make sure rn is updated after the first reg transfer
				}
			}
		}
		catch (uint32_t){
			//g_regs[rni] = ulTargetRn;	//should rn be updated when abort happens on the first reg transfer?
			RaiseExp(EXP_ABT, g_pc - 4);
			return 1;
		}
	}
	//g_regs[15] -= 4;
	return 0;
}

FASTCALL uint32_t Op_SWPB(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulSrc = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t ulAddr = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];

	g_usTicksThisPiece++;
	g_regs[15] += g_ulPcDelta;

	uint32_t foo = uint32_t(phym_read8(ulAddr));
	g_usTicksThisPiece++;
	phym_write8(ulAddr, uint8_t(ulSrc));
	g_regs[rdi] = foo;

	g_usTicksThisPiece++;
	//g_regs[15] -= 4;
	return 0;
}

FASTCALL uint32_t Op_SWPW(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulSrc = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
	uint32_t rdi = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t ulAddr = g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)];

	g_usTicksThisPiece++;
	g_regs[15] += g_ulPcDelta;

	uint32_t foo = phym_read32(ulAddr);
	g_usTicksThisPiece++;
	phym_write32(ulAddr & 0xFFFFFFFC/*not sure this is right*/, ulSrc);
	g_regs[rdi] = (foo >> ((ulAddr & 3UL) * 8)) | (foo << (32 - (ulAddr & 3UL) * 8));

	g_usTicksThisPiece++;
	//g_regs[15] -= 4;
	return 0;
}

FASTCALL uint32_t Op_MRS(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	uint32_t ulSrc = (ulOpCode & (1UL << 22)) == 0? g_cpsr: g_spsr;
	g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = ulSrc;
	return 0;
}

FASTCALL uint32_t Op_MSR(uint32_t ulOpCode)
{//this won't be done in user mode?
	TRACE_INSTR(ulOpCode, INT_BITS(uint32_t, ulOpCode, 0, 4));
	if ( (ulOpCode & (1UL << 22)) == 0 ){
		//g_cpsr = g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)];
		SwitchToMode(g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)]);
	}
	else
		g_spsr = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
	return 0;
}

FASTCALL uint32_t Op_MSRFR(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	if ( (ulOpCode & (1UL << 22)) == 0 )
		g_cpsr = (g_cpsr & 0x0FFFFFFF) | (g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)] & 0xF0000000);
	else
		g_spsr = (g_spsr & 0x0FFFFFFF) | (g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)] & 0xF0000000);
	return 0;
}

FASTCALL uint32_t Op_MSRFI(uint32_t ulOpCode)
{
	TRACE_INSTR(ulOpCode, 0);
	//the same as imm oprd2, without carry out, for even carry bit is set, it will be overwritten anyway
	uint32_t foo = ulOpCode & 0x000000FF;
	uint32_t samount = INT_BITS(uint32_t, ulOpCode, 8, 4) << 1;
	foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );

	if ( (ulOpCode & (1UL << 22)) == 0 )
		g_cpsr = (g_cpsr & 0x0FFFFFFF) | (foo & 0xF0000000);
	else
		g_spsr = (g_spsr & 0x0FFFFFFF) | (foo & 0xF0000000);
	return 0;
}
