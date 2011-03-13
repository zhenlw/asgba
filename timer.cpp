#include "phymem.h"
#include "interrupt.h"

struct TimerStatus{
	uint16 usCurr;
	uint16 usReload;
	uint16 usScale;
	uint16 usScaleLeftOver;
	bool bCountUp;
	bool bIntr;
} s_TimerStatus[4];

uint8_t s_TimerStatusOn[4];

static void TimerCtlSet(uint16_t idx)
{
	TimerStatus *pS = s_TimerStatus + idx;
	uint16_t usCtlAddr = 0x0102 + idx * 4;
    pS->usCurr = pS->usReload;
	switch ( REG_BITS(uint8_t, usCtlAddr, 0, 2) ){
		case 0:
			pS->usScale = 1;
			break;
		case 1:
			pS->usScale = 64;
			break;
		case 2:
			pS->usScale = 256;
			break;
		case 3:
			pS->usScale = 1024;
			break;
	}
	pS->usScaleLeftOver = 0;
	pS->bCountUp = ( REG_BITS(uint8_t, usCtlAddr, 2, 1) != 0 );
	pS->bIntr = ( REG_BITS(uint8_t, usCtlAddr, 6, 1) != 0 );

	s_TimerStatusOn[idx] = 1;
}

void DoTimerUpdate()	//do this after every cpu/dma piece
{
	if ( *(uint32_t*)s_TimerStatusOn == 0 )
		return;

	uint16_t usLowerOverflow = 0;
	for ( uint8_t i = 0; i != 4; i++ ){
		if ( s_TimerStatusOn[i] != 0 ){
			uint16_t usNewCnt = s_TimerStatus[i].usCurr;
			if ( s_TimerStatus[i].bCountUp )
				usNewCnt += usLowerOverflow;
			else{
				s_TimerStatus[i].usScaleLeftOver += g_usTicksThisPiece;
				usNewCnt += s_TimerStatus[i].usScaleLeftOver / s_TimerStatus[i].usScale;
				s_TimerStatus[i].usScaleLeftOver %= s_TimerStatus[i].usScale;
			}
			if ( usNewCnt < s_TimerStatus[i].usCurr ){	//overflow for this channel
				//should sound fifo event be risen?
				usLowerOverflow = 1;
				if ( s_TimerStatus[i].bIntr )
					SetIRQ(INTR_INDEX_TMR0 + i);
			}
			else
				usLowerOverflow = 0;
			s_TimerStatus[i].usCurr = usNewCnt;
		}
		else
			usLowerOverflow = 0;
	}
}

template<uint8_t byChnl, uint8_t byHigh>
static void TimerSetRel(uint8_t arrVal[], uint8_t size)
{
	*(uint8_t*(&(s_TimerStatus[byChnl].usReload)) + byHigh) = arrVal[0];
	//support little endian usReload only for now.
}

template<uint8_t byChnl, uint8_t byHigh>
static void TimerGetCnt(uint8_t size)
{
	g_arrDevRegCache[0x0100 + 4 * byChnl + byHigh] = *(uint8_t*(&(s_TimerStatus[byChnl].usCurr)) + byHigh);
}

template<uint8_t byChnl>
static void TimerSetCtl(uint8_t arrVal[], uint8_t size)
{
	g_arrDevRegCache[0x0102 + 4 * byChnl] = *arrVal;
	if ( arrVal[0] & 0x80!= 0 ) TimerCtlSet(byChnl);
	else{
		s_TimerStatusOn[byChnl] = 0;
	}
}


void Init_Timer()
{
    //the initial values are all zero for dma registers
	memset(s_TimerStatus, 0, sizeof(s_TimerStatus);
	memset(s_TimerStatusOn, 0, sizeof(s_TimerStatusOn);

	//register registers
	RegisterDevRegHandler(0x0100, TimerSetRel<0, 0>, TimerGetCnt<0, 0>);
	RegisterDevRegHandler(0x0101, TimerSetRel<0, 1>, TimerGetCnt<0, 1>);
	RegisterDevRegHandler(0x0104, TimerSetRel<1, 0>, TimerGetCnt<1, 0>);
	RegisterDevRegHandler(0x0105, TimerSetRel<1, 1>, TimerGetCnt<1, 1>);
	RegisterDevRegHandler(0x0108, TimerSetRel<2, 0>, TimerGetCnt<2, 0>);
	RegisterDevRegHandler(0x0109, TimerSetRel<2, 1>, TimerGetCnt<2, 1>);
	RegisterDevRegHandler(0x010C, TimerSetRel<3, 0>, TimerGetCnt<3, 0>);
	RegisterDevRegHandler(0x010D, TimerSetRel<3, 1>, TimerGetCnt<3, 1>);
	
	RegisterDevRegHandler(0x0102, TimerSetRel<0>);
	RegisterDevRegHandler(0x0106, TimerSetRel<1>);
	RegisterDevRegHandler(0x010A, TimerSetRel<2>);
	RegisterDevRegHandler(0x010E, TimerSetRel<3>);
}
