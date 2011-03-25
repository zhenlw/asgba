#ifndef PHYMEM_H_INCLUDED
#define PHYMEM_H_INCLUDED

#include "utilities.h"

//maybe handlers will raise exception; the params may be unnecessary
typedef void (*GetRegHandler_t)(uint8_t size);
typedef void (*SetRegHandler_t)(uint8_t arrVal[], uint8_t size);    //size=4,2,1

void RegisterDevRegHandler(uint32_t addr, SetRegHandler_t set, GetRegHandler_t get = NULL);

extern uint8_t g_arrDevRegCache[0x10000];
extern uint8_t g_arrStorVram[0x18000];
extern uint8_t g_arrStorPalram[0x400];

//we are not considering endians yet, since the target and host cpu both use little endian now.
#define REG_BITS(TYPE, ADDR, BITS_START, BITS_SZ)   INT_BITS(TYPE, (*(TYPE*)(g_arrDevRegCache + ADDR)), BITS_START, BITS_SZ)

//base memory management
//1k per block
struct BlockDesc {
    uint8_t *pBase;
    bool bDevReg;
    bool bWritable;
};

extern struct BlockDesc g_arrBlksDesc[];

FASTCALL uint8_t phym_read8(uint32_t addr) throw(uint32_t);
FASTCALL void phym_write8(uint32_t addr, uint8_t val) throw(uint32_t);
FASTCALL uint16_t phym_read16(uint32_t addr) throw(uint32_t);
FASTCALL void phym_write16(uint32_t addr, uint16_t val) throw(uint32_t);
FASTCALL uint32_t phym_read32(uint32_t addr) throw(uint32_t);
FASTCALL void phym_write32(uint32_t addr, uint32_t val) throw(uint32_t);

//FASTCALL void phym_FetchInstrs(uint32_t addr, uint32_t *pBuf) throw(uint32_t);

#endif // PHYMEM_H_INCLUDED
