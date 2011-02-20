/*
 * cpu_int_mul.cpp
 *
 *  Created on: 2011-2-10
 *      Author: zlw
 */

FASTCALL uint32_t Op_MUL(uint32_t ulOpCode)
{
	uint32_t rs = g_regs[INT_BITS(uint32_t, ulOpCode, 8, 4)];
	uint32_t rm = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];

	uint32_t ul = rs & 0x80000000;
	if ( 0UL - (ul >> 7) == (rs & 0xFF000000) ){
		if ( 0UL - (ul >> 15) == (rs & 0xFFFF0000) ){
			if ( 0UL - (ul >> 23) == (rs & 0xFFFFFF00) )
				g_ulTicksThisPiece++;
			else
				g_ulTicksThisPiece += 2;
		}
		else
			g_ulTicksThisPiece += 3;
	}
	else
		g_ulTicksThisPiece += 4;

	uint32_t foo = rm * rs;
	g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] = foo;
	if ( ulOpCode & (1UL << 20) != 0 ){
		g_cpsr = (g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		if ( foo == 0 )
			g_cpsr |= CPSR_FLAG_MASK_Z;
		else
			g_cpsr &= ~CPSR_FLAG_MASK_Z;
	}

	return 0;
}

FASTCALL uint32_t Op_MLA(uint32_t ulOpCode)
{
	uint32_t rs = g_regs[INT_BITS(uint32_t, ulOpCode, 8, 4)];
	uint32_t rm = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
	uint32_t rn = g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)];

	uint32_t ul = rs & 0x80000000;
	if ( 0UL - (ul >> 7) == (rs & 0xFF000000) ){
		if ( 0UL - (ul >> 15) == (rs & 0xFFFF0000) ){
			if ( 0UL - (ul >> 23) == (rs & 0xFFFFFF00) )
				g_ulTicksThisPiece += 2;
			else
				g_ulTicksThisPiece += 3;
		}
		else
			g_ulTicksThisPiece += 4;
	}
	else
		g_ulTicksThisPiece += 5;

	uint32_t foo = rm * rs + rn;
	g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] = foo;
	if ( ulOpCode & (1UL << 20) != 0 ){
		g_cpsr = (g_cpsr & ~CPSR_FLAG_MASK_N ) | ( res & CPSR_FLAG_MASK_N );
		if ( foo == 0 )
			g_cpsr |= CPSR_FLAG_MASK_Z;
		else
			g_cpsr &= ~CPSR_FLAG_MASK_Z;
	}

	return 0;
}

FASTCALL uint32_t Op_MULLU(uint32_t ulOpCode)
{
	uint32_t rs = g_regs[INT_BITS(uint32_t, ulOpCode, 8, 4)];
	uint32_t rm = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];

	uint32_t ul = rs & 0x80000000;
	if ( 0UL - (ul >> 7) == (rs & 0xFF000000) ){
		if ( 0UL - (ul >> 15) == (rs & 0xFFFF0000) ){
			if ( 0UL - (ul >> 23) == (rs & 0xFFFFFF00) )
				g_ulTicksThisPiece++;
			else
				g_ulTicksThisPiece += 2;
		}
		else
			g_ulTicksThisPiece += 3;
	}
	else
		g_ulTicksThisPiece += 4;

	uint64_t foo = uint64_t(rm) * uint64_t(rs);
	g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = uint32_t(foo);
	g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] = uint32_t(foo >> 32);
	if ( ulOpCode & (1UL << 20) != 0 ){
		g_cpsr = (g_cpsr & ~CPSR_FLAG_MASK_N ) | ( uint32_t(foo >> 32) & CPSR_FLAG_MASK_N );
		if ( foo == 0 )
			g_cpsr |= CPSR_FLAG_MASK_Z;
		else
			g_cpsr &= ~CPSR_FLAG_MASK_Z;
	}

	return 0;
}

FASTCALL uint32_t Op_MULLS(uint32_t ulOpCode)
{
	uint32_t rs = g_regs[INT_BITS(uint32_t, ulOpCode, 8, 4)];
	uint32_t rm = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];

	uint32_t ul = rs & 0x80000000;
	if ( 0UL - (ul >> 7) == (rs & 0xFF000000) ){
		if ( 0UL - (ul >> 15) == (rs & 0xFFFF0000) ){
			if ( 0UL - (ul >> 23) == (rs & 0xFFFFFF00) )
				g_ulTicksThisPiece++;
			else
				g_ulTicksThisPiece += 2;
		}
		else
			g_ulTicksThisPiece += 3;
	}
	else
		g_ulTicksThisPiece += 4;

	uint64_t foo = uint64_t(int64_t(int32_t(rm))) * uint64_t(int64_t(int32_t(rs)));
	g_regs[INT_BITS(uint32_t, ulOpCode, 12, 4)] = uint32_t(foo);
	g_regs[INT_BITS(uint32_t, ulOpCode, 16, 4)] = uint32_t(foo >> 32);
	if ( ulOpCode & (1UL << 20) != 0 ){
		g_cpsr = (g_cpsr & ~CPSR_FLAG_MASK_N ) | ( uint32_t(foo >> 32) & CPSR_FLAG_MASK_N );
		if ( foo == 0 )
			g_cpsr |= CPSR_FLAG_MASK_Z;
		else
			g_cpsr &= ~CPSR_FLAG_MASK_Z;
	}

	return 0;
}

FASTCALL uint32_t Op_MLALU(uint32_t ulOpCode)
{
	uint32_t rs = g_regs[INT_BITS(uint32_t, ulOpCode, 8, 4)];
	uint32_t rm = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
	uint32_t rdi1 = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t rdi2 = INT_BITS(uint32_t, ulOpCode, 16, 4);

	uint32_t ul = rs & 0x80000000;
	if ( 0UL - (ul >> 7) == (rs & 0xFF000000) ){
		if ( 0UL - (ul >> 15) == (rs & 0xFFFF0000) ){
			if ( 0UL - (ul >> 23) == (rs & 0xFFFFFF00) )
				g_ulTicksThisPiece++;
			else
				g_ulTicksThisPiece += 2;
		}
		else
			g_ulTicksThisPiece += 3;
	}
	else
		g_ulTicksThisPiece += 4;

	uint64_t foo = uint64_t(rm) * uint64_t(rs) + ((uint64_t(g_regs[rdi2]) << 32) | uint64_t(g_regs[rdi1]));
	g_regs[rdi1] = uint32_t(foo);
	g_regs[rdi2] = uint32_t(foo >> 32);
	if ( ulOpCode & (1UL << 20) != 0 ){
		g_cpsr = (g_cpsr & ~CPSR_FLAG_MASK_N ) | ( uint32_t(foo >> 32) & CPSR_FLAG_MASK_N );
		if ( foo == 0 )
			g_cpsr |= CPSR_FLAG_MASK_Z;
		else
			g_cpsr &= ~CPSR_FLAG_MASK_Z;
	}

	return 0;
}

FASTCALL uint32_t Op_MLALS(uint32_t ulOpCode)
{
	uint32_t rs = g_regs[INT_BITS(uint32_t, ulOpCode, 8, 4)];
	uint32_t rm = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
	uint32_t rdi1 = INT_BITS(uint32_t, ulOpCode, 12, 4);
	uint32_t rdi2 = INT_BITS(uint32_t, ulOpCode, 16, 4);

	uint32_t ul = rs & 0x80000000;
	if ( 0UL - (ul >> 7) == (rs & 0xFF000000) ){
		if ( 0UL - (ul >> 15) == (rs & 0xFFFF0000) ){
			if ( 0UL - (ul >> 23) == (rs & 0xFFFFFF00) )
				g_ulTicksThisPiece++;
			else
				g_ulTicksThisPiece += 2;
		}
		else
			g_ulTicksThisPiece += 3;
	}
	else
		g_ulTicksThisPiece += 4;

	uint64_t foo = uint64_t(int64_t(int32_t(rm))) * uint64_t(int64_t(int32_t(rs))) + ((uint64_t(g_regs[rdi2]) << 32) | uint64_t(g_regs[rdi1]));
	g_regs[rdi1] = uint32_t(foo);
	g_regs[rdi2] = uint32_t(foo >> 32);
	if ( ulOpCode & (1UL << 20) != 0 ){
		g_cpsr = (g_cpsr & ~CPSR_FLAG_MASK_N ) | ( uint32_t(foo >> 32) & CPSR_FLAG_MASK_N );
		if ( foo == 0 )
			g_cpsr |= CPSR_FLAG_MASK_Z;
		else
			g_cpsr &= ~CPSR_FLAG_MASK_Z;
	}

	return 0;
}
