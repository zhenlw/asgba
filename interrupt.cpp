/*
 * interupt.cpp
 *
 *  Created on: 2011-2-19
 *      Author: zlw
 */

#include "phymem.h"
#include "cpu.h"

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

static CRITICAL_SECTION s_csIRQ;
static volatile uint16_t s_usIRQ;

//other system registers
static void HaltCnt(uint8_t arrVal[], uint8_t size)
{
	//g_ulSleep = 1;
	if ( g_usTicksThisPiece < 100 ) g_usTicksThisPiece = 100;
}

void Init_INTR()
{
    //the initial values, all 3, are zero

	//register registers
	RegisterDevRegHandler(0x0202UL, IntrReq);	//the registers do the irq acknowledgment, I guess
	RegisterDevRegHandler(0x0203UL, IntrReq1);

	//other system registers
	RegisterDevRegHandler(0x0302UL, HaltCnt);

	InitializeCriticalSection(&s_csIRQ);
	s_usIRQ = 0;
}


void SetIRQ(uint8_t idx)	//this can run in different threads
{
	if ( idx < 14 ){
		EnterCriticalSection(&s_csIRQ);
		s_usIRQ |= uint16_t(1) << idx;
		LeaveCriticalSection(&s_csIRQ);
	}
}

FASTCALL void CheckIRQ()	//runs in cpu cycles only
{
	if ( s_usIRQ != 0 ){
		//new flag may enter during this time, but no problem
		if ( (g_arrDevRegCache[0x0208] & 0x01) != 0 ){
			EnterCriticalSection(&s_csIRQ);
			s_usIRQ &= *(uint16_t*)(g_arrDevRegCache + 0x200);
			if ( s_usIRQ != 0 ){
				*(uint16_t*)(g_arrDevRegCache + 0x202) |= s_usIRQ;
				s_usIRQ = 0;
				g_nirq = CPSR_FLAG_MASK_I;
			}
			LeaveCriticalSection(&s_csIRQ);
		}
		else{
			EnterCriticalSection(&s_csIRQ);
			s_usIRQ = 0;
			LeaveCriticalSection(&s_csIRQ);
		}
		g_nirq = CPSR_FLAG_MASK_I;
	}
}
