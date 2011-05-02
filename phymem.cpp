#include "phymem.h"

uint32_t s_dummy;
uint8_t g_arrStorBios[0x4000];   //start from addr 0x00000000
uint8_t g_arrStorRam[0x40000];  //start from addr 0x02000000
uint8_t g_arrStorIram[0x8000];  //start from addr 0x03000000
uint8_t g_arrStorPalram[0x400];  //start from addr 0x05000000
uint8_t g_arrStorVram[0x18000];  //start from addr 0x06000000
uint8_t g_arrStorOam[0x400];  //start from addr 0x07000000

uint8_t g_arrStorFlash[0x20000];   //128KB flash

struct BlockDesc g_arrBlksDesc[0x10000000/0x400];

//for convenience
uint16_t * const g_arrStorPalramBg = (uint16_t *)g_arrStorPalram;
uint16_t * const g_arrStorPalramSpr = (uint16_t *)(g_arrStorPalram + 0x200);

//device registers map
uint32_t s_dummy1;
uint8_t g_arrDevRegCache[0x10000];  //special one, only for caching
GetRegHandler_t g_arrDevRegGet[0x10000];
SetRegHandler_t g_arrDevRegSet[0x10000];
uint8_t g_arrDevRegReadable[0x10000];
uint8_t g_arrDevRegWritable[0x10000];


static void InitBlockArea(uint32_t ulAeraStart, uint32_t ulAeraSize, uint8_t *pSecBase, uint32_t ulSecSize, bool bWritable = true, bool bDevReg = false)
{
	struct BlockDesc *p = g_arrBlksDesc + (ulAeraStart >> 10);
	for ( uint32_t i = 0; i < ulAeraSize; i += 0x400, p++ ){
		p->bDevReg = bDevReg;
		p->bWritable = bWritable;
		p->pBase = ( pSecBase == NULL? NULL: pSecBase + (i % ulSecSize) );
	}
}

void PhyMemInit(uint8_t *bios, uint8_t *rom, uint32_t romsz)
{
    //main blocks desc
    InitBlockArea(0, sizeof(g_arrStorBios), g_arrStorBios, sizeof(g_arrStorBios), false);
    InitBlockArea(sizeof(g_arrStorBios), 0x02000000 - sizeof(g_arrStorBios), NULL, 0, false, false);
    InitBlockArea(0x02000000, 0x01000000, g_arrStorRam, sizeof(g_arrStorRam));
    InitBlockArea(0x03000000, 0x01000000, g_arrStorIram, sizeof(g_arrStorIram));
    InitBlockArea(0x04000000, 0x01000000, NULL, 0, true, true);
    InitBlockArea(0x05000000, 0x01000000, g_arrStorPalram, sizeof(g_arrStorPalram));
    InitBlockArea(0x06000000, 0x01000000, g_arrStorVram, sizeof(g_arrStorVram));
    InitBlockArea(0x07000000, 0x01000000, g_arrStorOam, sizeof(g_arrStorOam));
	InitBlockArea(0x08000000, 0x08000000, NULL, 0, false, false);
	InitBlockArea(0x08000000, romsz, rom, romsz, false);
	InitBlockArea(0x0A000000, romsz, rom, romsz, false);
	InitBlockArea(0x0C000000, romsz, rom, romsz, false);
	InitBlockArea(0x0A000000 - sizeof(g_arrStorFlash), sizeof(g_arrStorFlash), g_arrStorFlash, sizeof(g_arrStorFlash));
	InitBlockArea(0x0C000000 - sizeof(g_arrStorFlash), sizeof(g_arrStorFlash), g_arrStorFlash, sizeof(g_arrStorFlash));
	InitBlockArea(0x0E000000 - sizeof(g_arrStorFlash), sizeof(g_arrStorFlash), g_arrStorFlash, sizeof(g_arrStorFlash));

    //device registers realted
    for ( int32_t i = 0x0FFFF; i >= 0; i-- ){
        g_arrDevRegCache[i] = 0;
        g_arrDevRegGet[i] = NULL;
        g_arrDevRegSet[i] = NULL;
        g_arrDevRegReadable[i] = 1;
        g_arrDevRegWritable[i] = 1;
    }

    memcpy(g_arrStorBios, bios, sizeof(g_arrStorBios));
}

uint8_t phym_read8(uint32_t addr) throw(uint32_t)
{
    struct BlockDesc *pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        return *( pBD->pBase + ( addr & 0x3FF ) );
    }

    //registers
    if ( pBD->bDevReg && g_arrDevRegReadable[addr & 0xFFFFUL] ){
        if ( g_arrDevRegGet[addr & 0xFFFFUL] != NULL )
            g_arrDevRegGet[addr & 0xFFFFUL](1);
        return g_arrDevRegCache[addr & 0xFFFFUL];
    }
    throw addr;
}

void phym_write8(uint32_t addr, uint8_t val) throw(uint32_t)
{
	if ( addr >= 0x07000000 && addr < 0x08000000 ){
		addr = addr;
	}

    struct BlockDesc *pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        if ( pBD->bWritable ){
            *( pBD->pBase + ( addr & 0x3FF ) ) = val;
            return;
        }
        throw addr;
    }

    //registers, still need to be done in byte still, since the memory controller should do byte access correctly but not assume a 32-bit emulation
    if ( pBD->bDevReg && g_arrDevRegWritable[addr & 0xFFFFUL] ){
        //g_arrDevRegCache[addr & 0xFFFFUL] = val; //will there be any handler wants to avoid this writing?
        if ( g_arrDevRegSet[addr & 0xFFFFUL] != NULL ) g_arrDevRegSet[addr & 0xFFFFUL](g_arrDevRegCache + ( addr & 0xFFFFUL ), 1);
        else g_arrDevRegCache[addr & 0xFFFFUL] = val;
        return;
    }
    throw addr;	//enough info?
}

uint16_t phym_read16(uint32_t addr) throw(uint32_t)
{
	addr &= 0xFFFFFFFE;
    struct BlockDesc *pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        return *(uint16_t*)( pBD->pBase + ( addr & 0x3FF ) );
    }

    //registers
    uint32_t idx = addr & 0xFFFFUL;
    if ( pBD->bDevReg && (*(uint16_t*)(g_arrDevRegReadable + idx) == 0x0101) ){
        GetRegHandler_t *ph = g_arrDevRegGet + idx;
        if ( *ph != NULL ) (*ph)(2);
        ph++;
        if ( *ph != NULL ) (*ph)(2);
        return *(uint16_t*)(g_arrDevRegCache + idx);
    }
    throw addr;
}

void phym_write16(uint32_t addr, uint16_t val) throw(uint32_t)
{
	if ( addr == 0x07000020 ){	//DEBUG
		addr = addr;
	}
	else if ( addr == 0x03003648 ){
		addr = addr;
	}

	addr &= 0xFFFFFFFE;
    struct BlockDesc *pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        if ( pBD->bWritable ){
            *(uint16_t*)( pBD->pBase + ( addr & 0x3FF ) ) = val;
            return;
        }
        throw addr;
    }

    //registers
    uint32_t idx = addr & 0xFFFFUL;
    if ( pBD->bDevReg && (*(uint16_t*)(g_arrDevRegWritable + idx) == 0x0101) ){
        //*(uint16_t*)(g_arrDevRegCache + idx) = val; //will there be any handler wants to avoid this writing?
        SetRegHandler_t *ph = g_arrDevRegSet + idx;
        uint8_t *pSrc = (uint8_t*)&val;
        uint8_t *pTgt = g_arrDevRegCache + idx;

        if ( *ph != NULL )
            (*ph)(pSrc, 4);
        else
        	*pTgt = *pSrc;
        ph++; pSrc++; pTgt++;
        if ( *ph != NULL )
            (*ph)(pSrc, 4);
        else
        	*pTgt = *pSrc;
        return;
    }
    throw addr;
}

uint32_t phym_read32(uint32_t addr) throw(uint32_t)
{
	addr &= 0xFFFFFFFC;
    struct BlockDesc *pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        return *(uint32_t*)( pBD->pBase + ( addr & 0x3FF ) );
    }

    //registers
    uint32_t idx = addr & 0xFFFFUL;
    if ( pBD->bDevReg && (*(uint32_t*)(g_arrDevRegReadable + idx) == 0x01010101) ){
        GetRegHandler_t *ph = g_arrDevRegGet + idx;
        if ( *ph != NULL ) (*ph)(4);
        ph++;
        if ( *ph != NULL ) (*ph)(4);
        ph++;
        if ( *ph != NULL ) (*ph)(4);
        ph++;
        if ( *ph != NULL ) (*ph)(4);
        return *(uint32_t*)(g_arrDevRegCache + idx);
    }
    throw addr;
}

void phym_write32(uint32_t addr, uint32_t val) throw(uint32_t)
{
	if ( addr == 0x07000020 ){	//DEBUG
		addr = addr;
	}
	else if ( addr == 0x03003648 ){
		addr = addr;
	}

	addr &= 0xFFFFFFFC;
    struct BlockDesc *pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        if ( pBD->bWritable ){
            *(uint32_t*)( pBD->pBase + ( addr & 0x3FF ) ) = val;
            return;
        }
        throw addr;
    }

    //registers
    uint32_t idx = addr & 0xFFFFUL;
    if ( pBD->bDevReg && (*(uint32_t*)(g_arrDevRegWritable + idx) == 0x01010101) ){
        //*(uint32_t*)(g_arrDevRegCache + idx) = val; //will there be any handler wants to avoid this writing?
        SetRegHandler_t *ph = g_arrDevRegSet + idx;
        uint8_t *pSrc = (uint8_t*)&val;
        uint8_t *pTgt = g_arrDevRegCache + idx;

        if ( *ph != NULL )
            (*ph)(pSrc, 4);
        else
        	*pTgt = *pSrc;
        ph++; pSrc++; pTgt++;
        if ( *ph != NULL )
            (*ph)(pSrc, 4);
        else
        	*pTgt = *pSrc;
        ph++; pSrc++; pTgt++;
        if ( *ph != NULL )
            (*ph)(pSrc, 4);
        else
        	*pTgt = *pSrc;
        ph++; pSrc++; pTgt++;
        if ( *ph != NULL )
            (*ph)(pSrc, 4);
        else
        	*pTgt = *pSrc;
        return;
    }
    throw addr;
}

void RegisterDevRegHandler(uint32_t addr, SetRegHandler_t set, GetRegHandler_t get)
{
	g_arrDevRegGet[addr & 0xFFFFUL] = get;
	g_arrDevRegSet[addr & 0xFFFFUL] = set;
}
