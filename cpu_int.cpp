/*
 * cpu_int.cpp
 *
 *  Created on: 2011-1-29
 *      Author: zlw
 */

#include "phymem.h"
#include "cpu.h"

//ultils

typedef FASTCALL uint32_t (*OpFunc)(uint32_t);

//dispatch with the bits 20 to 27, and bits 4-9, so there is a 14 bits index and 16K entries mapping
static OpFunc s_OpDisp[0x4000];
static OpFunc* s_OpDisp2nd[0x4000];
static char (*s_OpDispPat2)[14];	//used/allocated at init only

FASTCALL uint32_t Func2ndDisp(uint32_t ulOpCode)
{
	return s_OpDisp2nd[((ulOpCode >> 14) & 0x3FC0) | ((ulOpCode >> 4) & 0x003F)]
		[((ulOpCode >> 6) & 0x3FF0) | (ulOpCode & 0x000F)](ulOpCode);
}

bool FillOpsI(const char arrPat[], OpFunc func, OpFunc disp[], const char arrPat2[])
{
	int a = 0;
	for ( int i = 0; i <= 13; i++ ) if ( arrPat[i] == 'x' ) a++;
	for ( uint16_t vpart = 0; vpart < ( 0x0001 << a ); vpart++ ){	//'a' cannot be larger than 12, but a 12 will cover the whole map
		//compose a 12-bit index
		uint16_t index = 0;
		int vbit_pos = 0;
		for ( int i = 0; i <= 13; i++ ){
			if ( arrPat[i] == '1' ) index |= 1 << i;
			else if ( arrPat[i] != '0' ) //0 need no filling
				index |= ( ( vpart >> vbit_pos++ ) & 0x0001 ) << i;
		}

		if ( arrPat2 != NULL ){	//level1
			if ( s_OpDisp[index] == NULL ){
				s_OpDisp[index] = func;
				memcpy(s_OpDispPat2[index], arrPat2, 14);	//for potential conflicts
			}
			else{
				if ( s_OpDisp2nd[index] == NULL ){
					s_OpDisp2nd[index] = new OpFunc[0x4000];
					memset(s_OpDisp2nd[index], 0, sizeof(OpFunc) * 0x4000);
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
	return true;
}

void FillOps(const char *szPattern, OpFunc func)
{
	//rearrange pattern
	char arrP1[14], arrP2[14];
	int i1 = 13, i2 = 13, i = 27;
	char ch;
	for ( const char *p = szPattern; *p != '\0'; p++ ){
		if ( *p == '-' || *p == '"' ) continue;
		if ( *p == '0' || *p == '1' )
			ch = *p;
		else
			ch = 'x';
		if ( i <= 27 && i >= 20 || i <= 9 && i >= 4 ){
			if ( i1 == -1 ) continue;
			arrP1[i1--] = ch;
		}
		else{
			if ( i2 == -1 ) continue;
			arrP2[i2--] = ch;
		}
		i--;
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
			if ( (i & 0x04) != 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 1:
			if ( (i & 0x04) == 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 2:
			if ( (i & 0x02) != 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 3:
			if ( (i & 0x02) == 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 4:
			if ( (i & 0x08) != 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 5:
			if ( (i & 0x08) == 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 6:
			if ( (i & 0x01) != 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 7:
			if ( (i & 0x01) == 0 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 8:
			if ( (i & 0x06) == 0x02 ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 9:
			if ( (i & 0x06) != 0x02 ) s_CondMap[i] = 1;
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
			if ( ( i & 0x04 ) == 0 && INT_BITS(uint16_t, i, 3, 1) == INT_BITS(uint16_t, i, 0, 1) ) s_CondMap[i] = 1;
			else s_CondMap[i] = 0;
			break;
		case 13:
			if ( ( i & 0x04 ) != 0 || INT_BITS(uint16_t, i, 3, 1) != INT_BITS(uint16_t, i, 0, 1) ) s_CondMap[i] = 1;
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

void CpuCycles()
{
	if ( (CPSR_FLAG_MASK_T & g_cpsr) == 0 ){	//arm state
		uint32_t ulOpCode;
		do{
			g_pc &= 0xFFFFFFFC;	//make sure it's 4 byte aligned: some instruction may set none aligned value
			//fetch
			try{
				ulOpCode = phym_read32(g_pc);	//replace this with batch fetching
			}
			catch (uint32_t){
				RaiseExp(EXP_PABT, 4);
				return; //end this bunch
			}
			g_usTicksThisPiece++;	//the instruction costs at least 1 cycle anyway
			g_pc += 8;	//for being executed instrs,current pc is always 8 bigger than its address
			//cond
			if ( s_CondMap[((ulOpCode >> 24) & 0xF0) | (g_cpsr >> 28)] != 0 ){
				//exec
				try{
					if ( 0 != s_OpDisp[((ulOpCode >> 14) & 0x3FC0) | ((ulOpCode >> 4) & 0x003F)](ulOpCode) )
						return;	//pc must be set correctly already, later this may be replaced by c++ exception
				}
				catch(uint32_t){
					RaiseExp(EXP_ABT, -4);
					return;
				}
			}
			g_pc -= 4;
		} while ( g_usTicksThisPiece < 100 );
	}
	else
		throw "thumb not implemented";
}

//fuctions
#include "cpu_int_dp.hpp"
#include "cpu_int_br.hpp"
#include "cpu_int_dt.hpp"
#include "cpu_int_mul.hpp"

//init
#define FillDatProc(OpCode, S, Func)\
	FillOps("00-0" OpCode S "xxxxxxxx-xxxx-xxx0-xxxx", Func);\
	FillOps("00-0" OpCode S "xxxxxxxx-xxxx-0xx1-xxxx", Func);\
	FillOps("00-1" OpCode S "xxxxxxxx-xxxx-xxxx-xxxx", Func);

class CpuIntMaintainer{
public:
	CpuIntMaintainer(){
		s_OpDispPat2 = new char[0x4000][14];
		memset(s_OpDisp, 0, sizeof(s_OpDisp));
		memset(s_OpDisp2nd, 0, sizeof(s_OpDisp2nd));

		InitCondMap();
	};
	~CpuIntMaintainer(){
		for (int32_t i = 0x04000 - 1; i > 0; i-- ){
			if ( s_OpDisp2nd[i] != NULL ) delete [] s_OpDisp2nd[i];
		}
	}
};

static CpuIntMaintainer s_CpuIntMaintainer;
//the operation dispatching, condition mapping thing should be run only once.

void InitCpu_()
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
		FillDatProc("1100", "s", Op_ORR);
		FillDatProc("1101", "s", Op_MOV);
		FillDatProc("1110", "s", Op_BIC);
		FillDatProc("1111", "s", Op_MVN);

		//branch
		FillOps("0001-0010-1111-1111-1111-0001-xxxx", Op_BX);
		FillOps("1010-xxxx-xxxx-xxxx-xxxx-xxxx-xxxx", Op_B);
		FillOps("1011-xxxx-xxxx-xxxx-xxxx-xxxx-xxxx", Op_BL);
		FillOps("1111-xxxx-xxxx-xxxx-xxxx-xxxx-xxxx", Op_SWI);

		//date transfer
		FillOps("00010-p-001111-xxxx-0000-0000-0000", Op_MRS);
		FillOps("00010-p-1010011111-00000000-xxxx", Op_MSR);
		FillOps("00010-p-1010001111-00000000-xxxx", Op_MSRFR);
		FillOps("00110-p-1010001111-xxxx-xxxxxxxx", Op_MSRFI);
		FillOps("01IP-U1W1-xxxx-xxxx-xxxxxxxxxxxx", Op_LDRB);
		FillOps("01IP-U0W1-xxxx-xxxx-xxxxxxxxxxxx", Op_LDRW);
		FillOps("01IP-U1W0-xxxx-xxxx-xxxxxxxxxxxx", Op_STRB);
		FillOps("01IP-U0W0-xxxx-xxxx-xxxxxxxxxxxx", Op_STRW);
		FillOps("000P-U0W1-xxxx-xxxx-0000-1011-xxxx", Op_LDRH);
		FillOps("000P-U0W1-xxxx-xxxx-0000-1111-xxxx", Op_LDRSH);
		FillOps("000P-U0W1-xxxx-xxxx-0000-1101-xxxx", Op_LDRSB);
		FillOps("000P-U0W0-xxxx-xxxx-0000-1011-xxxx", Op_STRH);
		FillOps("000P-U1W1-xxxx-xxxx-xxxx-1011-xxxx", Op_LDRH);
		FillOps("000P-U1W1-xxxx-xxxx-xxxx-1111-xxxx", Op_LDRSH);
		FillOps("000P-U1W1-xxxx-xxxx-xxxx-1101-xxxx", Op_LDRSB);
		FillOps("000P-U1W0-xxxx-xxxx-xxxx-1011-xxxx", Op_STRH);
		FillOps("100P-USW1-xxxx-xxxxxxxxxxxxxxxx", Op_LDM);
		FillOps("100P-USW0-xxxx-xxxxxxxxxxxxxxxx", Op_STM);
		FillOps("0001-0100-xxxx-xxxx-0000-1001-xxxx", Op_SWPB);
		FillOps("0001-0000-xxxx-xxxx-0000-1001-xxxx", Op_SWPW);

		//mul
		FillOps("0000-000S-xxxx-xxxx-xxxx-1001-xxxx", Op_MUL);
		FillOps("0000-001S-xxxx-xxxx-xxxx-1001-xxxx", Op_MLA);
		FillOps("0000-100S-xxxx-xxxx-xxxx-1001-xxxx", Op_MULLU);
		FillOps("0000-110S-xxxx-xxxx-xxxx-1001-xxxx", Op_MULLS);
		FillOps("0000-101S-xxxx-xxxx-xxxx-1001-xxxx", Op_MLALU);
		FillOps("0000-111S-xxxx-xxxx-xxxx-1001-xxxx", Op_MLALS);
		delete [] s_OpDispPat2;


}
