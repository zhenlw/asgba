#include "phymem.h"
#include "cpu.h"

//modes & status

const uint32_t STATE_THUMB = 0x20;
const uint32_t STATE_ARM = 0x0;

const uint32_t EXP_DISABLE_IF[EXP_FIQ + 1] = {0x80 + 0x40, 0, 0x80, 0, 0, 0, 0x80, 0x80 + 0x40};
const uint32_t EXP_TGT_MODE[EXP_FIQ + 1] = {MODE_SVC, MODE_UND, MODE_SVC, MODE_ABT, MODE_ABT, 0xFF, MODE_IRQ, MODE_FIQ};

uint32_t g_ulPcDelta;

//registers
uint32_t g_regsBak[MODE_SYS][17];	//the 17th is spsr. all registers are backed up here, but only banked are restore on swtiching-to
uint32_t g_cpsr;
uint32_t g_regs[17];
uint32_t &g_spsr = g_regs[16];
uint32_t &g_pc = g_regs[15];

//nirq flag
uint32_t g_nirq;	//set to CPSR_FLAG_MASK_I when irq happens

void CpuCycles();
void InitCpu_();

void SwitchRegs(uint32_t ulFrom, uint32_t ulTo)
{
	ulFrom %= 14;
	ulTo %= 14;
	if ( ulFrom != ulTo ){
		if ( ulFrom == MODE_FIQ ){	//backup & switch r8 through r12
			for (uint8_t i = 8; i <= 12; i++){
				g_regsBak[MODE_FIQ][i] = g_regs[i];
				g_regs[i] = g_regsBak[MODE_USR][i];
			}
		}
		else if ( ulTo == MODE_FIQ ){	//backup & switch r8 through r12
			for (uint8_t i = 8; i <= 12; i++){
				g_regsBak[MODE_USR][i] = g_regs[i];
				g_regs[i] = g_regsBak[MODE_FIQ][i];
			}
		}
		g_regsBak[ulFrom][13] = g_regs[13];
		g_regsBak[ulFrom][14] = g_regs[14];
		g_regs[13] = g_regsBak[ulTo][13];
		g_regs[14] = g_regsBak[ulTo][14];
		g_regsBak[ulFrom][16] = g_regs[16];	//user/sys modes have no spsr, but it's okay to save an unpredictable value here since it's never used
		g_regs[16] = g_regsBak[ulTo][16];
	}
}

void SwitchToMode(uint32_t ulNewCpsr)
{
	uint32_t ulCurrMode = g_cpsr & 0x0FUL;
	uint32_t ulToMode = ulNewCpsr & 0x0FUL;
	PRINT_TRACE("  ---SwitchMode: %d->%d, new cpsr = %X\n", ulCurrMode, ulToMode, ulNewCpsr);

	g_cpsr = ulNewCpsr;

    //save/restore banked registers
	SwitchRegs(ulCurrMode, ulToMode);
}

void RaiseExp(ExpType eType, uint32_t ulSavedPc)
{
    uint32_t ulCurrMode = g_cpsr & 0x0FUL;
    uint32_t ulToMode = EXP_TGT_MODE[eType];

    //save/restore banked registers
	SwitchRegs(ulCurrMode, ulToMode);

    g_spsr = g_cpsr;
    g_cpsr &= ~STATE_THUMB;
    g_cpsr |= EXP_DISABLE_IF[eType];
    g_cpsr = g_cpsr & 0xFFFFFFF0 | ( ulToMode + 0x10 );
    g_regs[14] = ulSavedPc;
    g_regs[15] = 0 + eType * 4;

    //count ticks up
    g_usTicksThisPiece += 2;
}

void BackFromExp(uint32_t ulNewPc)	//triggered on pc/r15 assigning with 'S' bit enabled
{
    uint32_t ulCurrMode = g_cpsr & 0x0FUL;	//cannot be user mode?
    uint32_t ulToMode = g_spsr & 0x0FUL;

    g_regs[15] = ulNewPc;	//g_regs[ulCurrMode][14] - ulPcDelta;
    g_cpsr = g_spsr;	//do this before restore of spsr

    //save/restore banked registers
	SwitchRegs(ulCurrMode, ulToMode);

    //count ticks up, just do the extra ticks it takes
	g_usTicksThisPiece += 2;
}

FASTCALL void CheckIRQ();
FASTCALL void DoDmaPiece();
FASTCALL void DoTimerUpdate();
FASTCALL bool DoDispTicksUpdate();

//sys ticks related, actually not real cpu work, but anyway
uint16_t g_usTicksThisPiece;
uint32_t g_ulTicksOa = 0;
bool bDyncTrace = false;

//main entrance, not only cpu work
void AsgbaExec()
{
    while (true) {
        g_usTicksThisPiece = 0;
        DoDmaPiece();
        if ( g_usTicksThisPiece == 0 )
            CpuCycles();

        //ticks processing, timer and video
        DoTimerUpdate();
        if ( DoDispTicksUpdate() == false )
			break;

        //irq are only checked here
        CheckIRQ();	//this is the check of the informal data to serialize the input in the cpu thread

        if ( ( g_cpsr & g_nirq ) != 0 ){	//this is the check of the formal flag
        	RaiseExp(EXP_IRQ, g_pc + 4);
        }
		g_ulTicksOa += g_usTicksThisPiece;
		PrintTrace("Overall ticks: %d (%X)\n", g_ulTicksOa);
    }
}

void InitCpu()
{
    //regs
	g_cpsr = 0;
	RaiseExp(EXP_RESET, g_pc);	//it happens to set the status right, the to be saved cpsr is "unpredictable" anyway

    //implement related
    InitCpu_();
}
