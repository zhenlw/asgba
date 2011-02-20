/*
 * interupt.cpp
 *
 *  Created on: 2011-2-19
 *      Author: zlw
 */

#include "phymem.h"


static void IntrReq(uint8_t arrVal[], uint8_t size)
{
	g_arrDevRegCache[0x0202] = (g_arrDevRegCache[0x0202] ^ arrVal[0]) & g_arrDevRegCache[0x0202];
	if ( size == 1 ){
		if ( *(uint16_t*)(g_arrDevRegCache+0x0202) == 0 ){	//the upper 2 bits are always 0
			//clear nirq
			g_nirq = 0;
		}
	}
}

static void IntrReq1(uint8_t arrVal[], uint8_t size)
{
	g_arrDevRegCache[0x0203] = (g_arrDevRegCache[0x0203] ^ arrVal[0]) & g_arrDevRegCache[0x0203];
	if ( *(uint16_t*)(g_arrDevRegCache+0x0202) == 0 ){
		g_nirq = 0;
	}
}

void Init_INTR()
{
    //the initial values, all 3, are zero

	//register registers
	RegisterDevRegHandler(0x0202UL, IntrReq);	//the registers do the irq acknowledgment, I guess
	RegisterDevRegHandler(0x0203UL, IntrReq1);
}

void SetNIRQ(uint8_t idx)	//this can run in different threads
{
	//lock
	*(uint16_t*)(g_arrDevRegCache + 0x202) != uint16_t(1) << idx;
	g_nirq = CPSR_FLAG_MASK_I;
	//unlock
}
