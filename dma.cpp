#include "phymem.h"


static void DoDma(uint32_t ulBaseRegAddr)
{
    uint32_t ulAddrS = REG_BITS(uint32_t, ulBaseRegAddr, 0, 27);
    uint32_t ulAddrD = REG_BITS(uint32_t, ulBaseRegAddr + 4, 0, 27);
    struct BlockDesc *pBD = g_arrBlksDesc + ( ulAddrD >> 10 );
    uint16_t flag;
    int32_t lDesInc;
    uint16_t *pDes;
    if ( pBD->bDevReg ){    //sound FIFO, but right?
        lDesInc = 0;
    }
    else{
        flag = REG_BITS(uint16_t, ulBaseRegAddr + 0x0A, 5, 2);
        if ( flag == 0b11 || flag == 0 ) lDesInc = 1;
        else if ( flag == 0b01 ) lDesInc = -1;
        else lDesInc = 0;
    }
    int32_t lSrcInc;
    flag = REG_BITS(uint16_t, ulBaseRegAddr + 0x0A, 7, 2);
    //if ( flag == 0b11 )   //exception?
    if ( flag == 0 ) lSrcInc = 1;
    else if ( flag == 0b01 ) lSrcInc = -1;
    else lDesInc = 0;

    //destination/src address, granularity, count
    uint32_t ulCount= REG_BITS(uint16_t, ulBaseRegAddr + 0x08, 0, 14);
    if ( ulCount == 0 ) ulCount = 2 ^ 14;

    //do the transfer, the honest but very slow way first
    if ( 0 == REG_BITS(uint16_t, ulBaseRegAddr + 0x0A, 10, 1) ){
        lDesInc *= 2;
        lSrcInc *= 2;
        for ( ; ulCount--; ulCount > 0 ){
            phym_write16(ulAddrD, phym_read16(ulAddrS));	//exception processing
            ulAddrS += lSrcInc;
            ulAddrD += lDesInc;
        }
    }
    else{
        lDesInc *= 4;
        lSrcInc *= 4;
        for ( ; ulCount--; ulCount > 0 ){
            phym_write32(ulAddrD, phym_read32(ulAddrS));
            ulAddrS += lSrcInc;
            ulAddrD += lDesInc;
        }
    }

    //clear up: enable bit? maybe an interupt?
}

static void dma0ctl(uint8_t arrVal[], uint8_t size)
{
    if ( g_arrStorRegCache[0x0BB] & 0x80 != 0 ){    //starting, or should the previous state be checked
        uint8_t flag = REG_BITS(uint8_t, 0x0BB, 4, 2);
        if ( flag == 0 )
            DoDma(0xB0);
        //else just do nothing, let the vblank/hblank events handle it
    }
    //else just do nothing?
}
void Init_DMA()
{
    //the initial values are all zero for dma registers

	//register registers
	RegisterDevRegHandler(0xBBUL, dma0ctl);	//B0-BB
}
