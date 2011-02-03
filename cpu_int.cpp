/*
 * cpu_int.cpp
 *
 *  Created on: 2011-1-29
 *      Author: zlw
 */

//ultils
#define FASTCALL __fastcall

typedef FASTCALL uint32_t (*OpFunc)(uint32_t);

//dispatch with the bits 20 to 27, and bits 4-7, so there is a 12 bits index and 4K entries mapping
static OpFunc s_OpDisp[0x1000];
static (OpFunc *) s_OpDispLv2[0x1000];
static char s_OpDispPat2[][12];	//used/allocated at init only

FASTCALL uint32_t Func2ndDisp(uint32_t);

bool FillOpsI(const char arrPat[], OpFunc func, OpFunc disp[], const char arrPat2[])
{
	int a = 0;
	for ( int i = 0; i <= 11; i++ ) if ( arrPat[i] == 'x' ) a++;
	for ( uint16_t vpart = 0; vpart < 2 ^ a; vpart++ ){	//'a' cannot be larger than 12, but a 12 will cover the whole map
		//compose a 12-bit index
		uint16_t index = 0;
		int vbit_pos = 0;
		for ( int i = 0; i <= 11; i++ ){
			if ( arrPat[i] == '1' ) index |= 1 << i;
			else if ( arrPat[i] != '0' ) //0 need no filling
				index |= ( ( vpart >> vbit_pos++ ) & 0x0001 ) << i;
		}

		if ( arrPat2 != NULL ){	//level1
			if ( s_OpDisp[index] == NULL ){
				s_OpDisp[index] = func;
				memcpy(s_OpDispPat2[index], arrPat2);	//for potential conflicts
			}
			else{
				if ( s_OpDisp2nd[index] == NULL ){
					s_OpDisp2nd[index] = new OpFunc[0x1000];
					memzero(s_OpDisp2nd[index], sizeof(OpFunc));
					FillOpsI(s_OpDispPat2[index], s_OpDisp[index], s_OpDisp2nd[index], NULL);	//the first one should not cause any problem
					s_OpDisp[index] = Func2ndDisp;
					//s_OpDispPat2[index] is of no use now.
				}
				if ( !FillOpsI(arrPat2, func, s_OpDisp2nd[index], NULL) ) return false;
			}
		}
		else{//level 2
			if ( disp[index] != NULL ) return false;
			disp[index] = func;
		}
	}
}

void FillOps(const char *szPattern, OpFunc func)
{
	//rearrange pattern
	char arrP1[12], arrP2[12];
	int i1 = 11, i2 = 11, i = 27;
	for ( const char *p = szPattern; *p != '\0'; p++ ){
		if ( *p == '-' || *p == '"' ) continue;
		if ( i <= 27 && i >= 20 || i <= 7 && i >= 4 ){
			if ( i1 == -1 ) continue;
			arrP1[i1--] = *p;
		}
		else{
			if ( i2 == -1 ) continue;
			arrP2[i2--] = *p;
		}
	}
	if ( i1 != -1 || i2 != -1 ) return;

	//go through the 1st level pattern
	FillOpsI(arrP1, func, NULL, arrP2);
}

static uint8_t s_CondMap[256];

static void InitCondMap()
{
	for ( uint16_t i = 0; i <= 255; i++ ){
		switch ( ( i & 0x00F0 ) >> 4 ){
		case 0:
			if ( i & 0b00000100 != 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 1:
			if ( i & 0b00000100 == 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 2:
			if ( i & 0b00000010 != 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 3:
			if ( i & 0b00000010 == 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 4:
			if ( i & 0b00001000 != 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 5:
			if ( i & 0b00001000 == 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 6:
			if ( i & 0b00000001 != 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 7:
			if ( i & 0b00000001 == 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 8:
			if ( i & 0b00000110 == 0b0010 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 9:
			if ( i & 0b00000110 != 0b0010 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 10:
			if ( INT_BITS(uint16_t, i, 3, 1) == INT_BITS(uint16_t, i, 0, 1) ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 11:
			if ( INT_BITS(uint16_t, i, 3, 1) != INT_BITS(uint16_t, i, 0, 1) ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 12:
			if ( ( i & 0b00000100 ) == 0 && INT_BITS(uint16_t, i, 3, 1) == INT_BITS(uint16_t, i, 0, 1) ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 13:
			if ( ( i & 0b00000100 ) != 0 || INT_BITS(uint16_t, i, 3, 1) != INT_BITS(uint16_t, i, 0, 1) ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 14:
			s_CondMap[i] = 1;
			break;
		default:
			s_CondMap[i] = 0;	//should not be a problem to be either 0 or 1
			break;
		}
	}
}

//fuctions
static uint32_t ulCarryOut;

uint32_t DataProcOpd2(uint32_t ulOpCode)
{
	uint32_t foo, samount;
	if ( ulOpCode & (1 << 25) == 0 ){	//reg oprd 2
		foo = g_regs[INT_BITS(uint32_t, ulOpCode, 0, 4)];
		if ( ulOpCode & (1 << 4) == 0 ){
			samount = INT_BITS(uint32_t, ulOpCode, 7, 5);
			switch ( INT_BITS(uint32_t, ulOpCode, 5, 2) ){
			case 0b00:
				if ( samount == 0 ){
					ulCarryOut = CPSR_FLAG_MASK_C & g_cpsr;
					return foo;
				}
				foo <<= ( samount - 1 );
				ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				return foo << 1;
			case 0b01:
				if ( samount == 0 ) samount = 32;
				foo >>= samount - 1;
				ulCarryOut = ( foo  & 0x01 ) << 29;
				return foo >> 1;
			case 0b10:
				if ( samount == 0 ) samount = 32;
				foo = uint32_t( int32_t(foo) >> ( samount - 1 ) );
				ulCarryOut = ( foo  & 0x01 ) << 29;
				return uint32_t( int32_t(foo) >> 1 );
			case 0b11:
				if ( samount == 0 ){
					ulCarryOut = ( foo  & 0x01 ) << 29;
					return ( foo >> 1 ) | ( ( g_cpsr & CPSR_FLAG_MASK_C ) << (31 - 29) );
				}
				foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
				ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				return foo;
			}
		}
		else{
			g_ulTicksThisPiece++;	//extra cycle needed for the 3rd reg
			samount = g_regs[INT_BITS(uint32_t, ulOpCode, 8, 4)] & 0xFF;
			if ( samount == 0 ){
				ulCarryOut = CPSR_FLAG_MASK_C & g_cpsr;
				return foo;
			}
			switch ( INT_BITS(uint32_t, ulOpCode, 5, 2) ){
			case 0b00:
				foo <<= ( samount - 1 );
				ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				return foo << 1;
			case 0b01:
				foo >>= samount - 1;
				ulCarryOut = ( foo  & 0x01 ) << 29;
				return foo >> 1;
			case 0b10:
				foo = uint32_t( int32_t(foo) >> ( samount - 1 ) );
				ulCarryOut = ( foo  & 0x01 ) << 29;
				return uint32_t( int32_t(foo) >> 1 );
			case 0b11:
				samount %= 32;
				foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
				ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );
				return foo;
			}
		}
	}
	else{	//immd oprd2
		foo = ulOpCode & 0x000000FF;
		samount = INT_BITS(uint32_t, ulOpCode, 8, 4) << 1;
		if ( samount == 0 ){
			ulCarryOut = CPSR_FLAG_MASK_C & g_cpsr;	//do we need carry out on immd oprd2?
			return foo;
		}
		foo = ( foo >> samount ) | ( foo << ( 32 - samount ) );
		ulCarryOut = ( foo & 0x80000000 ) >> ( 31 - 29 );	//do we need carry out on immd oprd2?
		return foo;
	}
}

#include "cpu_int_dp.cpp"

//init
#define FillDatProc(OpCode, S, Func)\
	FillOps("00-0"##OpCode##S##"xxxxxxxx-xxx0-xxxx", Func);\
	FillOps("00-0"##OpCode##S##"xxxxxxxx-0xx1-xxxx", Func);\
	FillOps("00-1"##OpCode##S##"xxxxxxxx-xxxx-xxxx", Func);


void InitCpuInt()
{
	//fill in the dispatching array
	//FillOps("00-0-0000-s-xxxxxxxx-xxx0-xxxx", Op_And_Reg);

	FillDatProc("0000", "s", Op_AND);
	FillDatProc("0001", "s", Op_EOR);
	FillDatProc("0010", "s", Op_SUB);
	FillDatProc("0011", "s", Op_RSB);
	FillDatProc("0100", "s", Op_ADD);
	FillDatProc("0101", "s", Op_ADC);
	FillDatProc("0110", "s", Op_SBC);
	FillDatProc("0111", "s", Op_RSC);
	FillDatProc("1000", "1", Op_TST);
	FillDatProc("1001", "1", Op_TEQ);
	FillDatProc("1010", "1", Op_CMP);
	FillDatProc("1011", "1", Op_CMN);
	FillDatProc("0100", "s", Op_ORR);
	FillDatProc("0101", "s", Op_MOV);
	FillDatProc("0110", "s", Op_BIC);
	FillDatProc("0111", "s", Op_MVN);


}
