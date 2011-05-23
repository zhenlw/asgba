/*
 * cpu_int.cpp
 *
 *  Created on: 2011-1-29
 *      Author: zlw
 */

#include "phymem.h"
#include "cpu.h"

//utils
uint32_t ulInstrAddr;	//be global for the sake of tracing
#ifdef TRACE_ON
#define TRACE_INSTR(ARG1, ARG2)	PRINT_TRACE("---ARM---%X: %s %X %X\n", ulInstrAddr, __FUNCTION__, uint32_t(ARG1), uint32_t(ARG2))
#define TRACE_INSTR_THUMB(ARG1, ARG2)	PRINT_TRACE("--THUMB--%X: %X %s\n", ulInstrAddr, uint32_t(ARG1), ARG2)
#else
#define TRACE_INSTR(ARG1, ARG2)
#define TRACE_INSTR_THUMB(ARG1, ARG2)
#endif

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

//thumb utilities
class ThumbInstrConverter{
private:
	static uint32_t m_arrRuntimeTable[0x10000UL];	//later the function should also be directly mapped to save some calculation
	static const char *m_arrRuntimeDescTable[0x10000UL];
	static struct LiteralMap {
		const char *szThumb;
		const char *szDesc;
		const char *szArm;
	} m_arrMap[];
public:
	static bool Init();
	static uint32_t ToArmOpCode(uint16_t usThumbOpCode){
		TRACE_INSTR_THUMB(usThumbOpCode, m_arrRuntimeDescTable[usThumbOpCode]);
		return m_arrRuntimeTable[usThumbOpCode];
	};
};

uint32_t ThumbInstrConverter::m_arrRuntimeTable[0x10000UL];
const char *ThumbInstrConverter::m_arrRuntimeDescTable[0x10000UL];

ThumbInstrConverter::LiteralMap ThumbInstrConverter::m_arrMap[] = {
		"000-aa-bbbbb-sss-ddd", "shift & mov", //aa != 11"
			"1110-000-1101-1-0000-0ddd-bbbbbaa0-0sss",

		"00011-i0-nnn-sss-ddd", "add", //the pattern conflicts, the later one get priority
			"1110-00i-0100-1-0sss-0ddd-00000000-0nnn",	//happen to be able to hold both 3bit imm & reg

		"00011-i1-nnn-sss-ddd", "sub",
			"1110-00i-0010-1-0sss-0ddd-00000000-0nnn",	//happen to be able to hold both 3bit imm & reg

		"001-00-ddd-nnnnnnnn", "mov imm",
			"1110-001-1101-1-0000-0ddd-0000-nnnnnnnn",

		"001-01-sss-nnnnnnnn", "cmp imm",
			"1110-001-1010-1-0sss-0000-0000-nnnnnnnn",

		"001-10-ddd-nnnnnnnn", "add imm",
			"1110-001-0100-1-0ddd-0ddd-0000-nnnnnnnn",

		"001-11-ddd-nnnnnnnn", "sub imm",
			"1110-001-0010-1-0ddd-0ddd-0000-nnnnnnnn",

		//two registers alu operations
		"010000-oooo-sss-ddd", "common 2 reg alu ops",
			"1110-000-oooo-1-0ddd-0ddd-00000000-0sss",

		"010000-0010-sss-ddd", "rd = rd << rs",
			"1110-000-1101-1-0000-0ddd-0sss-0-00-1-0ddd",

		"010000-0011-sss-ddd", "rd = rd >> rs",
			"1110-000-1101-1-0000-0ddd-0sss-0-01-1-0ddd",

		"010000-0100-sss-ddd", "rd = rd ASR rs",
			"1110-000-1101-1-0000-0ddd-0sss-0-10-1-0ddd",

		"010000-0111-sss-ddd", "rd = rd ROR rs",
			"1110-000-1101-1-0000-0ddd-0sss-0-11-1-0ddd",

		"010000-1001-sss-ddd", "rd = 0 - rs",
			"1110-001-0011-1-0sss-0ddd-0000-00000000",	//a RSB with imm

		"010000-1101-sss-ddd", "rd = rs * rd",
			"1110-000000-01-0ddd-0000-0ddd-1001-0sss", //a MUL

		//hi/low
		"010001-00-ab-sss-ddd", "add [hi reg]",
			"1110-000-0100-0-addd-addd-00000000-bsss",

		"010001-01-ab-sss-ddd", "cmp [hi reg]",
			"1110-000-1010-1-addd-addd-00000000-bsss",

		"010001-10-ab-sss-ddd", "mov [hi reg]",
			"1110-000-1101-0-0000-addd-00000000-bsss",

		"010001-11-0b-sss-ddd", "bx [hi reg]",	//allow ddd == casual numbers, but may not be most accurate
			"1110-0001-0010-1111-1111-1111-0001-bsss",

		//ldr/str
		"01001-ddd-nnnnnnnn", "ldr [pc + offset]",
			"1110-01-011001-1111-0ddd-00nnnnnnnn00",	//imm offset, pre, up, word, no wb, ldr

		"0101-Lw0-ooobbbddd", "ldr/str [rb + ro]",
			"1110-01-111w0L-0bbb-0ddd-00000000-0ooo",	//reg offset, pre, up, word/byte, no wb, str/ldr

		"0101-L01-ooobbbddd", "ldr/str [rb + ro] halfword",
			"1110-000-1100L-0bbb-0ddd-0000-1011-0ooo",	//pre, up, , no wb, str/ldr

		"0101-H11-ooobbbddd", "ldr B/H [rb + ro] singed extension",
			"1110-000-11001-0bbb-0ddd-0000-11H1-0ooo",	//pre, up, , no wb, ldr

		"011-1L-ooooo-bbbddd", "ldrb/strb [rb + #ooooo]",
			"1110-01-01110L-0bbb-0ddd-0000000ooooo",	//imm offset, pre, up, byte, no wb, ldr/str

		"011-0L-ooooo-bbbddd", "ldrw/strw [rb + #ooooo00]",
			"1110-01-01100L-0bbb-0ddd-00000ooooo00",	//imm offset, pre, up, word, no wb, ldr/str

		"1000-L-oonnn-bbbddd", "ldrH/strH [rb + #ooooo0]",
			"1110-000-1110L-0bbb-0ddd-00oo-1011-nnn0",	//pre, up, , no wb, str/ldr

		"1001-L-ddd-nnnnnnnn", "ldr/str [sp + offset]",
			"1110-01-01100L-1101-0ddd-00nnnnnnnn00",	//imm offset, pre, up, word, no wb, ldr/str

		//load address
		"1010-1-ddd-oooooooo", "ldA (sp + offset)",
			"1110-001-0100-0-1101-0ddd-1111-oooooooo",	//add, ror 30 to make 10 bits offset

		"1010-0-ddd-oooooooo", "ldA (pc + offset)",
			"1110-001-0100-0-1111-0ddd-1111-oooooooo",	//add, ror 30 to make 10 bits offset

		//r13 increase
		"10110000-0-ooooooo", "add sp, offset",
			"1110-001-0100-0-1101-1101-1111-0ooooooo",	//add, ror 30 to make 9 bits offset

		"10110000-1-ooooooo", "sub sp, offset",
			"1110-001-0010-0-1101-1101-1111-0ooooooo",	//add, ror 30 to make 9 bits offset

		//push/pop regs
		"1011-010-r-nnnnnnnn", "push, stm dec before",
			"1110-100-10010-1101-0r000000-nnnnnnnn",	//pre, down, no mode force, wb, str

		"1011-110-r-nnnnnnnn", "pop, ldm inc afer",
			"1110-100-01011-1101-r0000000-nnnnnnnn",	//post, up, no mode force, wb, ldr

		//stm/ldm inc after
		"1100-L-bbb-nnnnnnnn", "stm/ldm inc after",
			"1110-100-0101L-0bbb-00000000-nnnnnnnn",	//post, up, no mode force, wb, str/ldr

		//cond branch, cccc != 1110 or 1111
		"1101-cccc-sooooooo", "cond branch",
			"cccc-1010-ssss-ssss-ssssssss-sooooooo",

		//SWI, conflict with cond branch pattern, the later one gets priority
		"1101-1111-nnnnnnnn", "SWI",
			"1110-1111-0000-0000-00000000-nnnnnnnn",

		//uncond branch
		"11100-soo-oooooooo", "uncond branch",
			"1110-1010-ssss-ssss-ssssssoo-oooooooo",

		//uncond BL far, no arm equivalent, disp to normal bl, no conflicts with other thumb branches.
		"1111-hooo-oooooooo", "uncond BL",
			"1110-1011-0000-0000-0000hooo-oooooooo"
};

bool ThumbInstrConverter::Init()
{
	//prepare the matching pattern
	uint16_t arrMatch[sizeof(m_arrMap)/sizeof(m_arrMap[0])][2];
	for ( size_t i1 = 0; i1 < sizeof(m_arrMap)/sizeof(m_arrMap[0]); i1++){
		uint16_t usMatchMask = 0, usMatchPat = 0;
		const char *p = m_arrMap[i1].szThumb;
		int16_t sPos = 15;
		while ( *p != 0 && sPos >= 0 ){
			if ( *p == '0' || *p == '1' ){
				usMatchMask |= uint16_t(1) << sPos;
				usMatchPat |= uint16_t(*p - '0') << sPos;
				sPos--;
			}
			else if ( *p >= 'a' && *p <= 'z' ){
				sPos--;
			}
			else if ( *p >= 'A' && *p <= 'Z' ){
				sPos--;
			}
			p++;
		}
		arrMatch[i1][0] = usMatchMask;
		arrMatch[i1][1] = usMatchPat;
	}

	//the real deal
	uint16_t i = 0;
	do{
		int j = int(sizeof(m_arrMap)/sizeof(m_arrMap[0])) - 1;
		for ( ; j >= 0; j--){
			if ( (i & arrMatch[j][0]) == arrMatch[j][1] ){	//match, compose the target uint32
				uint32_t ulVars[26][3];	//0-the curr val, 1-the total count, 2-the current count
				memset(ulVars, 0, sizeof(ulVars));
				//collect vars first
				const char *p = m_arrMap[j].szThumb;
				int16_t sPos = 15;
				while ( *p != 0 && sPos >= 0 ){
					if ( *p == '0' || *p == '1' ){
						sPos--;
					}
					else if ( *p >= 'a' && *p <= 'z' ){
						ulVars[*p - 'a'][0] = ( ulVars[*p - 'a'][0] << 1 ) + INT_BITS(uint16_t, i, sPos, 1);
						ulVars[*p - 'a'][1]++;
						sPos--;
					}
					else if ( *p >= 'A' && *p <= 'Z' ){
						ulVars[*p - 'A'][0] = ( ulVars[*p - 'A'][0] << 1 ) + INT_BITS(uint16_t, i, sPos, 1);
						ulVars[*p - 'A'][1]++;
						sPos--;
					}
					p++;
				}
				//compose
				p = m_arrMap[j].szArm;
				sPos = 31;
				uint32_t ulCmd = 0;
				while ( *p != 0 && sPos >= 0 ){
					char idx = -1;
					if ( *p == '0' || *p == '1' ){
						ulCmd |= uint32_t(*p - '0') << sPos;
						sPos--;
					}
					else if ( *p >= 'a' && *p <= 'z' ){
						idx = *p - 'a';
					}
					else if ( *p >= 'A' && *p <= 'Z' ){
						idx = *p - 'A';
					}
					if ( idx != -1 ){
						if ( ulVars[idx][1] == 0 ) return false;
						ulCmd |= INT_BITS(uint32_t, ulVars[idx][0], ulVars[idx][1] - ulVars[idx][2] - 1, 1) << sPos;
						ulVars[idx][2] = ( ulVars[idx][2] + 1 ) % ulVars[idx][1];
						sPos--;
					}
					p++;
				}
				m_arrRuntimeTable[i] = ulCmd;
				m_arrRuntimeDescTable[i] = m_arrMap[j].szDesc;
				break;
			}
		}
		if ( j < 0 ){	//no match, init to "illegal"
			m_arrRuntimeTable[i] = 0x07FFFFFF;
			m_arrRuntimeDescTable[i] = "illegal";
		}
		i++;
	} while (i != 0);
	return true;
}

/*extern uint32_t g_ulTicksOa;
bool bRom = false;
*/
//main entry for interpreted cpu
void CpuCycles()
{
	//if ( g_ulTicksOa > 0x048c9000 ) bDyncTrace = true;
	uint32_t ulOpCode;
	if ( (CPSR_FLAG_MASK_T & g_cpsr) == 0 ){	//arm state
		g_ulPcDelta = 4;
		ulInstrAddr = g_pc & 0xFFFFFFFC;	//make sure it's 4 byte aligned: some instruction may set none aligned value
		/*if ( ulInstrAddr == 0x08000000 ){
			//bDyncTrace = true;
			//g_ulTicksOa = 0;
			bRom = true;
			PrintTrace("entering rom\n");
		}
		/*if ( bRom ){
			if ( g_ulTicksOa > 10000 )
				g_ulTicksOa = 0;
		}*/
		do{
			//fetch
			try{
				ulOpCode = phym_read32(ulInstrAddr);	//replace this with batch fetching
			}
			catch (uint32_t){
				RaiseExp(EXP_PABT, ulInstrAddr + 4);
				return; //end this bunch
			}
			g_usTicksThisPiece++;	//the instruction costs at least 1 cycle anyway
			g_pc = ulInstrAddr + 8;	//for being executed instrs,current pc is always 8 bigger than its address
			//cond
			if ( s_CondMap[((ulOpCode >> 24) & 0xF0) | (g_cpsr >> 28)] != 0 ){
				//exec
				try{
					if ( s_OpDisp[((ulOpCode >> 14) & 0x3FC0) | ((ulOpCode >> 4) & 0x003F)](ulOpCode) != 0 ) return;
				}
				catch(uint32_t){
					RaiseExp(EXP_ABT, ulInstrAddr + 8);	//the instru address + 8
					return;
				}
			}
			ulInstrAddr += 4;
		} while ( g_usTicksThisPiece < MAX_TICKS_PER_CPU_PIECE );
		g_pc = ulInstrAddr;
	}
	else{
		g_ulPcDelta = 2;
		ulInstrAddr = g_pc & 0xFFFFFFFE;	//make sure it's 2 byte aligned: some instruction may set none aligned value?
		uint16_t usOpCode;
		do{
			//fetch
			try{
				usOpCode = phym_read16(ulInstrAddr);	//replace this with batch fetching
			}
			catch (uint32_t){
				RaiseExp(EXP_PABT, ulInstrAddr + 4);
				return; //end this bunch
			}
			g_usTicksThisPiece++;	//the instruction costs at least 1 cycle anyway
			g_pc = ulInstrAddr + 4;	//for being executed instrs,current pc is always 4 bigger than its address
			ulOpCode = ThumbInstrConverter::ToArmOpCode(usOpCode);
			//cond
			if ( s_CondMap[((ulOpCode >> 24) & 0xF0) | (g_cpsr >> 28)] != 0 ){
				//exec
				try{
					if ( s_OpDisp[((ulOpCode >> 14) & 0x3FC0) | ((ulOpCode >> 4) & 0x003F)](ulOpCode) != 0 ) return;
				}
				catch(uint32_t){
					RaiseExp(EXP_ABT, ulInstrAddr + 8);	//the instru address + 8
					return;
				}
			}
			ulInstrAddr += 2;
		} while ( g_usTicksThisPiece < MAX_TICKS_PER_CPU_PIECE );
		g_pc = ulInstrAddr;
	}
}

//fuctions
#include "cpu_int_dp.hpp"
#include "cpu_int_br.hpp"
#include "cpu_int_dt.hpp"
#include "cpu_int_mul.hpp"

//init
//the arm instrs
#define FillDatProc(OpCode, S, Func)\
	FillOps("00-0" OpCode S "xxxxxxxx-xxxx-xxx0-xxxx", Func);\
	FillOps("00-0" OpCode S "xxxxxxxx-xxxx-0xx1-xxxx", Func);\
	FillOps("00-1" OpCode S "xxxxxxxx-xxxx-xxxx-xxxx", Func);

bool InitArmOps()
{
	s_OpDispPat2 = new char[0x4000][14];
	memset(s_OpDisp, 0, sizeof(s_OpDisp));
	memset(s_OpDisp2nd, 0, sizeof(s_OpDisp2nd));

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
	return true;
}

class CpuIntMaintainer{
public:
	CpuIntMaintainer(){
		InitArmOps();
		InitCondMap();
		if ( !ThumbInstrConverter::Init() ) throw "init failed";
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
}
