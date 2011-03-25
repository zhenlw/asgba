#include "asgba.h"

void InitCpu();
void PhyMemInit(uint8_t *bios);
void Init_DMA();
void Init_Display();
void Init_INTR();
void Init_Timer();

//extern uint8_t g_arrRndrBuf[160][240][3];
AsgbaEvtHdlr g_funcOutEvtHdlr;

bool AsgbaInit(AsgbaEvtHdlr hdlr, uint8_t *bios)
{
	PhyMemInit(bios);
	InitCpu();
	Init_DMA();
	Init_INTR();
	Init_Display();
	Init_Timer();

	g_funcOutEvtHdlr = hdlr;
	return true;
}