#include "asgba.h"

void InitCpu();
void PhyMemInit(uint8_t *bios, uint8_t *rom, uint32_t romsz);
void Init_DMA();
void Init_Display(uint8_t *rgbbuf);
void Init_INTR();
void Init_Timer();

//extern uint8_t g_arrRndrBuf[160][240][3];
AsgbaEvtHdlr g_funcOutEvtHdlr;

bool AsgbaInit(AsgbaEvtHdlr hdlr, uint8_t *bios, uint8_t *rom, uint32_t romsz, uint8_t *rgbbuf)
{
	PhyMemInit(bios, rom, romsz);
	InitCpu();
	Init_DMA();
	Init_INTR();
	Init_Display(rgbbuf);
	Init_Timer();

	g_funcOutEvtHdlr = hdlr;
	return true;
}