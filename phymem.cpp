#include <stdint.h>

uint8_t g_arrStorRom[0x4000];   //start from addr 0x00000000
uint8_t g_arrStorRam[0x40000];  //start from addr 0x02000000
uint8_t g_arrStorIram[0x8000];  //start from addr 0x03000000
uint8_t g_arrStorPalram[0x400];  //start from addr 0x05000000
uint8_t g_arrStorVram[0x18000];  //start from addr 0x06000000
uint8_t g_arrStorOam[0x400];  //start from addr 0x07000000

//1k per block
struct BlockDesc {
    uint8_t *pBase;
    uint8_t bDevReg;
    uint8_t bWritable;
};

struct BlockDesc g_arrBlksDesc[0x10000000/0x400];

//regeisters related
uint8_t g_arrStorRegCache[0x10000];  //special one, only for caching
GetRegHandler_t g_arrGetHdls[0x10000];
SetRegHandler_t g_arrSetHdls[0x10000];
uint8_t g_arrRegReadable[0x10000];
uint8_t g_arrRegWritable[0x10000];


static void InitBlockArea(uint32_t ulAeraStart, uint32_t ulAeraSize, uint8_t *pSecBase, uint32_t ulSecSize, uint8_t bWritable = 1, uint8_t bDevReg = 0, uint8_t bRepSec = 1);

void PhyMemInit()
{
    //blocks desc
    InitBlockArea(0, 0x02000000, g_arrStorRom, sizeof(g_arrStorRom), 0, 0, 0);
    InitBlockArea(0x02000000, 0x01000000, g_arrStorRam, sizeof(g_arrStorRam));
    InitBlockArea(0x03000000, 0x01000000, g_arrStorIram, sizeof(g_arrStorIram));
    InitBlockArea(0x04000000, 0x01000000, NULL, 0, 1, 1, 0);
    InitBlockArea(0x05000000, 0x01000000, g_arrStorPalram, sizeof(g_arrStorPalram));
    InitBlockArea(0x06000000, 0x01000000, g_arrStorVram, sizeof(g_arrStorVram));
    InitBlockArea(0x07000000, 0x01000000, g_arrStorOam, sizeof(g_arrStorOam));

    //reg realted
    for ( int32_t i = 0x0FFFF; i >= 0; i-- ){
        g_arrGetHdls[i] = NULL;
        g_arrSetHdls[i] = NULL;
        g_arrStorRegCache[i] = 0;
        g_arrRegReadable[i] = 1;
        g_arrRegWritable[i] = 1;
    }

    //init actions from devices
    Init_DMA();
}

EXP_STATE phym_read8(uint32_t addr, uint8_t *pVal)
{
    struct BlockDesc *pBD;
    pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        *pVal = *( pBD + ( addr & 0x3FF ) )
        return EXP_OK;
    }

    //registers
    if ( g_arrRegReadable[addr & 0xFFFF] ){
        if ( g_arrGetHdls[addr & 0xFFFF] != NULL )
            g_arrGetHdls[addr & 0xFFFF](g_arrStorRegCache + ( addr & 0xFFFF ), 1);
        *pVal = g_arrStorRegCache[addr & 0xFFFF];
        return EXP_OK;
    }
    return EXP_EXP_MEM_READ;
}

EXP_STATE phym_write8(uint32_t addr, uint8_t val)
{
    struct BlockDesc *pBD;
    pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        if ( pDB->bWritable ){
            *( pBD + ( addr & 0x3FF ) ) = val;
            return EXP_OK;
        }
        return EXP_EXP_MEM_WRITE;
    }

    //registers
    if ( g_arrRegWritable[addr & 0xFFFF] ){
        g_arrStorRegCache[addr & 0xFFFF] = val; //will there be any handler wants to avoid this writing?
        if ( g_arrSetHdls[addr & 0xFFFF] != NULL ) g_arrSetHdls[addr & 0xFFFF](g_arrStorRegCache + ( addr & 0xFFFF ), 1);
        return EXP_OK;
    }
    return EXP_EXP_MEM_WRITE;
}

EXP_STATE phym_read16(uint32_t addr, uint16_t *pVal)
{
    struct BlockDesc *pBD;
    pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        *pVal = *(uint16_t*)( pBD + ( addr & 0x3FF ) )
        return EXP_OK;
    }

    //registers
    uint32_t idx = addr & 0xFFFF;
    if ( *(uint16_t*)(g_arrRegReadable + idx) == 0x0101 ){
        GetRegHandler_t *ph = g_arrGetHdls + idx;
        if ( *ph != NULL )
            (*ph)(g_arrStorRegCache + idx, 2);
        if ( *(ph + 1) != NULL )
            (*(ph + 1))(g_arrStorRegCache + idx, 2);
        *pVal = *(uint16_t*)(g_arrStorRegCache + idx);
        return EXP_OK;
    }
    return EXP_EXP_MEM_READ;
}

EXP_STATE phym_write16(uint32_t addr, uint16_t val)
{
    struct BlockDesc *pBD;
    pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        if ( pDB->bWritable ){
            *(uint16_t*)( pBD + ( addr & 0x3FF ) ) = val;
            return EXP_OK;
        }
        return EXP_EXP_MEM_WRITE;
    }

    //registers
    uint32_t idx = addr & 0xFFFF;
    if ( *(uint16_t*)(g_arrRegWritable + idx) == 0x0101 ){
        *(uint16_t*)(g_arrStorRegCache Ŕ`+ idx) = val; //will there be any handler wants to avoid this writing?
        GetRegHandler_t *ph = g_arrGetHdls + idx;
        if ( *ph != NULL )
            (*ph)(g_arrStorRegCache + idx, 2);
        if ( *(ph + 1) != NULL )
            (*(ph + 1))(g_arrStorRegCache + idx, 2);
        return EXP_OK;
    }
    return EXP_EXP_MEM_WRITE;
}

EXP_STATE phym_read32(uint32_t addr, uint32_t *pVal)
{
    struct BlockDesc *pBD;
    pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        *pVal = *(uint16_t*)( pBD + ( addr & 0x3FF ) )
        return EXP_OK;
    }

    //registers
    uint32_t idx = addr & 0xFFFF;
    if ( *(uint32_t*)(g_arrRegReadable + idx) == 0x01010101 ){
        GetRegHandler_t *ph = g_arrGetHdls + idx;
        if ( *ph != NULL )
            (*ph)(g_arrStorRegCache + idx, 4);
        if ( *(ph + 1) != NULL )
            (*(ph + 1))(g_arrStorRegCache + idx, 4);
        if ( *(ph + 2) != NULL )
            (*(ph + 2))(g_arrStorRegCache + idx, 4);
        if ( *(ph + 3) != NULL )
            (*(ph + 3))(g_arrStorRegCache + idx, 4);
        *pVal = *(uint32_t*)(g_arrStorRegCache + idx);
        return EXP_OK;
    }
    return EXP_EXP_MEM_READ;
}

EXP_STATE phym_write32(uint32_t addr, uint32_t val)
{
    struct BlockDesc *pBD;
    pBD = g_arrBlksDesc + ( addr >> 10 );
    if ( pBD->pBase != NULL ){
        if ( pDB->bWritable ){
            *(uint32_t*)( pBD + ( addr & 0x3FF ) ) = val;
            return EXP_OK;
        }
        return EXP_EXP_MEM_WRITE;
    }

    //registers
    uint32_t idx = addr & 0xFFFF;
    if ( *(uint32_t*)(g_arrRegWritable + idx) == 0x0101 ){
        *(uint32_t*)(g_arrStorRegCache Ŕ`+ idx) = val; //will there be any handler wants to avoid this writing?
        GetRegHandler_t *ph = g_arrGetHdls + idx;
        if ( *ph != NULL )
            (*ph)(g_arrStorRegCache + idx, 4);
        if ( *(ph + 1) != NULL )
            (*(ph + 1))(g_arrStorRegCache + idx, 4);
        if ( *(ph + 2) != NULL )
            (*(ph + 2))(g_arrStorRegCache + idx, 4);
        if ( *(ph + 3) != NULL )
            (*(ph + 3))(g_arrStorRegCache + idx, 4);
        return EXP_OK;
    }
    return EXP_EXP_MEM_WRITE;
}

