#include <phymem.h>

//modes & status
const uint32 MODE_USR = 0; //ignore the 5th 1 for easier operation
const uint32 MODE_FIQ = 1;
const uint32 MODE_IRQ = 2;
const uint32 MODE_SVC = 3;
const uint32 MODE_ABT = 7;
const uint32 MODE_UND = 0xB;
const uint32 MODE_SYS = 0xF;

const uint32 STATE_THUMB = 0x20;
const uint32 STATE_ARM = 0x0;

const uint32 EXP_DISABLE_IF[EXP_FIQ + 1] = {0x80 + 0x40, 0, 0x80, 0, 0, 0, 0x80, 0x80 + 0x40};
const uint32 EXP_TGT_MODE[EXP_FIQ + 1] = {MODE_SVC, MODE_UND, MODE_SVC, MODE_ABT, MODE_ABT, 0xFF, MODE_IRQ, MODE_FIQ};

//registers
uint32_t g_regsBak[MODE_SYS][17];	//the 17th is spsr. all registers are backed up here, but only banked are restore on swtiching-to
uint32_t g_cpsr;
uint32_t g_regs[17];
uint32_t &g_spsr = g_regs[16];
uint32_t &g_pc = g_regs[15];

//nirq flag
uint32_t g_nirq;	//set to CPSR_FLAG_MASK_I when irq happens

//ticks elapsed
uint32_t g_ulTicks;
uint32_t g_ulTicksThisPiece;

uint32_t g_ulNextTicksRendH;
uint32_t g_ulNextTicksRendV;
uint32_t g_ulNextTicksBlankH;
uint32_t g_ulNextTicksBlankV;

void CpuCycles();
void InitCpu_();

inline void SwitchRegs(uint32_t ulFrom, uint32_t ulTo)
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

	g_cpsr = ulNewCpsr;

    //save/restore banked registers
	SwitchRegs(ulCurrMode, ulToMode);
}

void RaiseExp(ExpType eType, int32_t ulPcDelta)
{
    uint32_t ulCurrMode = g_cpsr & 0x0FUL;
    uint32_t ulToMode = EXP_TGT_MODE[eType];

    //save/restore banked registers
	SwitchRegs(ulCurrMode, ulToMode);

    g_spsr = g_cpsr;
    g_cpsr &= ~STATE_THUMB;
    g_cpsr |= EXP_DISABLE_IF[eType];
    g_cpsr = g_cpsr & 0xFFFFFFF0 | ( ulToMode + 0x10 );
    g_regs[14] = g_regs[15] + ulPcDelta;
    g_regs[15] = 0 + eType * 4;

    //count ticks up
    g_ulTicksThisPiece += 2;
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
	g_ulTicksThisPiece += 2;
}

void Exec()
{
    g_ulTicksThisPiece = 0;
    while (true) {
        CpuCycles(); //cpu cycles may trigger dma too

        //ticks processing
        g_ulTicks += g_ulTicksThisPiece;
        //should we allow ticks based actions happen before exceptions?
        if ( g_ulTicks >= g_ulNextTicksRendH ){
            if ( g_ulTicks >= g_ulNextTicksRendV ){
                //clear v count. may be do a syncing with real world time later
                *(uint16_t *)(g_arrStorRegCache + 6) = 0;
                g_ulNextTicksRendV += 1232 * 228; //280948;  //16.743ms * 16.78Mhz
                //g_ulNextTicksRendH = g_ulNextTicksRendV + 1232;
                //g_ulNextTicksBlankH = g_ulNextTicksRendV + 960;  //57.221us * 16.78Mhz
                //g_ulNextTicksBlankV = g_ulNextTicksRendV + 197148;   //11.749ms * 16.78Mhz
            }
            else{
                if ( g_ulTicks >= g_ulNextTicksBlankV ){
                    //vblank int/dma

                    g_ulNextTicksBlankV += 1232 * 228; //280948;  //16.743ms * 16.78Mhz
                }
                *(uint16_t *)(g_arrStorRegCache + 6) += 1;
                g_ulNextTicksRendH += 1232;  //73.433us * 16.78Mhz
            }

            DisplayRenderLine();
        }
        else if ( g_ulTicks >= g_ulNextTicksBlankH ){
            //hblank int/dma

            g_ulNextTicksBlankH += 1232;  //73.433us * 16.78Mhz
        }

        //irq are only checked here
        if ( g_cpsr & g_nirq != 0 ){
        	RaiseExp(EXP_IRQ, 4);
        }
    }
}

void InitCpu()
{
    //regs
	g_cpsr = 0;
	RaiseExp(EXP_RESET, 0);	//it happens to set the status right, the to be saved cpsr is "unpredictable" anyway

    //ticks
    g_ulTicks = 0;
    g_ulNextTicksRendV = 0;
    g_ulNextTicksRendH = g_ulNextTicksRendV + 1232;
    g_ulNextTicksBlankH = g_ulNextTicksRendV + 960;  //57.221us * 16.78Mhz
    g_ulNextTicksBlankV = g_ulNextTicksRendV + 160 * 1232; //197148;   //11.749ms * 16.78Mhz

    //implement related
    InitCpu_();
}
