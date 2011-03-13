#include "phymem.h"
#include "interrupt.h"

struct DmaStatus{
	enum {Stopped = 0, Running, Stalled} eState;	//actual state
	bool bRepeat;
	bool bSet;
	bool b32bit;
	bool bReloadDesOnRepeat;
	SystemEvent eStartEvent;
	uint32_t ulCount;
	uint32_t ulAddrS;
	uint32_t ulAddrD;
	int32_t lIncS;
	int32_t lIncD;

} s_DmaStatus[4];

static void DmaSet(uint16_t idx)
{
	DmaStatus *pS = s_DmaStatus + idx;
	uint16_t usBsAddr = 0xB0 + idx * 12;
    pS->ulAddrS = REG_BITS(uint32_t, usBsAddr, 0, 27);
    pS->ulAddrD = REG_BITS(uint32_t, usBsAddr + 4, 0, 27);
    struct BlockDesc *pBD = g_arrBlksDesc + ( pS->ulAddrD >> 10 );
    uint16_t flag;

	if ( 0 == REG_BITS(uint16_t, usBsAddr + 0x09, 10, 1) )
		pS->bRepeat = false;
	else
		pS->bRepeat = true;

	flag = REG_BITS(uint16_t, usBsAddr + 0x0A, 12, 2);
	if ( flag == 0 ){
		pS->eState = pS->Running;
		pS->eStartEvent = EVT_NONE;
		//what if bRepeat == true & startmode == 0? supress bRepeat?
		pS->bRepeat = false;
	}
	else{
		pS->eState = pS->Stopped;
		pS->eStartEvent = SystemEvent(flag);	//TODO: 0b11 should be prohibited on dma 0, and video cap on dma 4
	}
	//what if bRepeat == false & StartMode != 0? exec once at next triger? assume that, and check bRepeat on the end of one dma

	pS->bReloadDesOnRepeat = false;
    if ( pBD->bDevReg && pS->bRepeat && ( flag == 3 ) && ( idx == 1 || idx == 2 ) ){    //sound FIFO, but right?
        pS->lIncD = 0;
		pS->ulCount = 4;
		pS->b32bit = true;
    }
    else{
        flag = REG_BITS(uint16_t, usBsAddr + 0x0A, 5, 2);
		if ( flag == 0b11 ){
			pS->lIncD = 1;
			pS->bReloadDesOnRepeat = true;
		}
		else if ( flag == 0 ) pS->lIncD = 1;
        else if ( flag == 0b01 ) pS->lIncD = -1;
        else pS->lIncD = 0;

	    //granularity, count
		if ( 0 == REG_BITS(uint16_t, usBsAddr + 0x0A, 10, 1) )
			pS->b32bit = false;
		else
			pS->b32bit = true;
		pS->ulCount = REG_BITS(uint16_t, usBsAddr + 0x08, 0, 14);
	    if ( pS->ulCount == 0 ) pS->ulCount = 0x01UL << 14;
    }

	//source increment
    flag = REG_BITS(uint16_t, usBsAddr + 0x0A, 7, 2);
    //if ( flag == 0b11 )   //exception?
	if ( flag == 0 ) pS->lIncS = 1;
    else if ( flag == 0b01 ) pS->lIncS = -1;
    else pS->lIncS = 0;

    //adjust per granularity
	if ( pS->b32bit ){
        pS->lIncD *= 4;
        pS->lIncS *= 4;
    }
    else{
        pS->lIncD *= 2;
        pS->lIncS *= 2;
    }

	pS->bSet = true;
}

void DmaEvent(SystemEvent e)
{
	if ( e != EVT_NONE ){	//this may not be necessary, depending on how the function is called
		DmaStatus *pS = s_DmaStatus;
		uint16_t usBsAddr = 0xB0;
		for ( uint8_t i = 4; i != 0; i-- ){
			if ( pS->bSet && pS->eState == DmaStatus::Stopped
					&& pS->eStartEvent == e ){
				//reload count/dest addr
				if ( e == EVT_FIFO ){
					pS->ulCount = 4;
				}
				else{
					pS->ulCount = REG_BITS(uint16_t, usBsAddr + 0x08, 0, 14);
					if ( pS->ulCount == 0 ) pS->ulCount = 0x01UL << 14;
					if ( pS->bReloadDesOnRepeat ){
						pS->ulAddrD = REG_BITS(uint32_t, usBsAddr + 4, 0, 27);
					}
				}
				pS->eStartEvent == DmaStatus::Running;
				//break;
			}
			pS++;
			usBsAddr += 12;
		}
	}
}

void DoDmaPiece()
{
	DmaStatus *pS = s_DmaStatus;
	for ( uint8_t i = 4; i != 0; i-- ){
		if ( s_DmaStatus[i].eState == DmaStatus::Running ){
			//do a piece of transfer
			uint32_t cnt = pS->ulCount;
			if ( cnt > 32 ) cnt = 32;
			pS->ulCount -= cnt;
			g_usTicksThisPiece += cnt * 2;
			if ( pS->b32bit ){
				for ( ; cnt--; cnt > 0 ){
					phym_write32(pS->ulAddrD, phym_read32(pS->ulAddrS));
					pS->ulAddrS += pS->lIncS;
					pS->ulAddrD += pS->lIncD;
				}
			}
			else{
				for ( ; cnt--; cnt > 0 ){
					phym_write16(pS->ulAddrD, phym_read16(pS->ulAddrS));
					pS->ulAddrS += pS->lIncS;
					pS->ulAddrD += pS->lIncD;
				}
			}
			//ending process
			if ( pS->ulCount == 0 ){
				pS->eState = DmaStatus::Stopped;
				uint16_t usBsAddr = 0x00B0 + ( 4 - i ) * 12;
				if ( !pS->bRepeat ){
					pS->bSet = false;
					g_arrDevRegCache[usBsAddr + 11] &= 0x7F;
				}
				if ( REG_BITS(uint16_t, usBsAddr + 0x0A, 14, 1) != 0 )
					SetIRQ(INTR_INDEX_DMA0 + 4 - i);
			}
			break;
		}
		pS++;
	}
}

static void dma0ctl(uint8_t arrVal[], uint8_t size)
{
	g_arrDevRegCache[0xBB] = *arrVal;
    if ( arrVal[0] & 0x80 != 0 ){    //starting, or should the previous state be 0 to start the dma?
									//The setting loading is a must. the starting is at least very possible at 1->1. So assume dma starts always
        DmaSet(0);
    }
    else{
		s_DmaStatus[0].eState = DmaStatus::Stopped;
		s_DmaStatus[0].bSet = false;
	}
}

static void dma1ctl(uint8_t arrVal[], uint8_t size)
{
	g_arrDevRegCache[0xC7] = *arrVal;
    if ( arrVal[0] & 0x80 != 0 ){
        DmaSet(1);
    }
    else{
		s_DmaStatus[1].eState = DmaStatus::Stopped;
		s_DmaStatus[1].bSet = false;
	}
}

static void dma2ctl(uint8_t arrVal[], uint8_t size)
{
	g_arrDevRegCache[0xD3] = *arrVal;
    if ( arrVal[0] & 0x80 != 0 ){    //starting, or should the previous state be checked? seems not: a start should be done anyway
        DmaSet(2);
    }
    else{
		s_DmaStatus[2].eState = DmaStatus::Stopped;
		s_DmaStatus[2].bSet = false;
	}
}

static void dma3ctl(uint8_t arrVal[], uint8_t size)
{
	g_arrDevRegCache[0xDF] = *arrVal;
    if ( arrVal[0] & 0x80 != 0 ){    //starting, or should the previous state be checked? seems not: a start should be done anyway
        DmaSet(3);
    }
    else{
		s_DmaStatus[3].eState = DmaStatus::Stopped;
		s_DmaStatus[3].bSet = false;
	}
}

void Init_DMA()
{
    //the initial values are all zero for dma registers

	//register registers
	RegisterDevRegHandler(0xBBUL, dma0ctl);	//B0-BB
	RegisterDevRegHandler(0xC7UL, dma1ctl);
	RegisterDevRegHandler(0xD3UL, dma2ctl);
	RegisterDevRegHandler(0xDFUL, dma3ctl);
}
