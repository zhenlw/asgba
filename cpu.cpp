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
uint32_t g_regsBak[MODE_SYS + 1][17];	//the 17th is spsr. all registers are backed up here, but only banked are restore on swtiching-to
uint32_t g_cpsr;
uint32_t g_regs[17];
uint32_t &g_spsr = g_regs[16];

//ticks elapsed
uint32_t g_ulTicks;
uint32_t g_ulTicksThisPiece;

uint32_t g_ulNextTicksRendH;
uint32_t g_ulNextTicksRendV;
uint32_t g_ulNextTicksBlankH;
uint32_t g_ulNextTicksBlankV;

void CpuCycles();

void SwitchToMode(uint32_t ulToMode)
{
	uint32_t ulCurrMode = g_cpsr & 0x0F;

	g_cpsr = g_cpsr & 0xFFFFFFF0 | ( ulToMode + 0x10 );

    //save/restore banked registers
    if ( ulToMode != ulCurrMode ){
    	//copy the current registers back
    	memcpy(g_regsBak[ulCurrMode], g_regs, sizeof(g_regs));
    	//copy banked registers from the target mode
    	if ( ulToMode == MODE_FIQ || ulCurrMode == MODE_FIQ ) memcpy(g_regCurr + 8, g_regsBak[ulToMode] + 8, 7 * sizeof(uint32_t));
    	else memcpy(g_regCurr + 13, g_regsBak[ulToMode] + 13, 2 * sizeof(uint32_t));
    	g_regs[16] = g_regsBak[ulToMode][16];	//banked spsr, though mode 0 doesn't have spsr
    }

}

void RaiseExp(ExpType eType, uint32_t ulPcDelta)
{
    uint32_t ulCurrMode = g_cpsr & 0x0F;
    uint32_t ulToMode = EXP_TGT_MODE[eType];

    //save/restore banked registers
    if ( ulToMode != ulCurrMode ){
    	//copy the current registers back
    	memcpy(g_regsBak[ulCurrMode], g_regs, sizeof(g_regs));
    	//copy banked registers from the target mode, note we cannot trap to FIQ mode
    	if ( ulCurrMode == MODE_FIQ ) memcpy(g_regCurr + 8, g_regsBak[ulToMode] + 8, 7 * sizeof(uint32_t));
    	else memcpy(g_regCurr + 13, g_regsBak[ulToMode] + 13, 2 * sizeof(uint32_t));
    	//no spsr restore: will be overwritten anyway
    }

    g_spsr = g_cpsr;
    g_cpsr &= ~STATE_THUMB;
    g_cpsr |= EXP_DISABLE_IF[eType];
    g_cpsr = g_cpsr & 0xFFFFFFF0 | ( ulToMode + 0x10 );
    g_regs[14] = g_regs[15] + ulPcDelta;
    g_regs[15] = 0 + eType * 4;

    //count ticks up
}

void BackFromExp(uint32_t ulNewPc)	//triggered on pc/r15 assigning with 'S' bit enabled
{
    uint32_t ulCurrMode = g_cpsr & 0x0F;	//cannot be user mode?
    uint32_t ulToMode = g_spsr & 0x0F;

    g_regs[15] = ulNewPc;	//g_regs[ulCurrMode][14] - ulPcDelta;
    g_cpsr = g_spsr;	//do this before restore of spsr

    //save/restore banked registers
    if ( ulToMode != ulCurrMode ){
    	//copy the current registers back
    	memcpy(g_regsBak[ulCurrMode], g_regs, sizeof(g_regs));
    	//copy banked registers from the target mode
    	if ( ulToMode == MODE_FIQ || ulCurrMode == MODE_FIQ ) memcpy(g_regCurr + 8, g_regsBak[ulToMode] + 8, 7 * sizeof(uint32_t));
    	else memcpy(g_regCurr + 13, g_regsBak[ulToMode] + 13, 2 * sizeof(uint32_t));
    	g_regs[16] = g_regsBak[ulToMode][16];	//banked spsr, though mode 0 doesn't have spsr
    }

    //count ticks up, just do the extra ticks it takes
	g_ulTicksThisPiece += 2;
}

void ExecPiece()
{
    g_ulTicksThisPiece = 0;
    {
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

        //do int? or should int be done by the raising & go through the cpu cycles natually?
        //at least input event should be handled here for serialized operation
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

}
