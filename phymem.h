#ifndef PHYMEM_H_INCLUDED
#define PHYMEM_H_INCLUDED

//maybe later handlers should be able to raise exception
typedef void (*GetRegHandler_t)(uint8_t arrVal[], uint8_t size);
typedef void (*SetRegHandler_t)(uint8_t arrVal[], uint8_t size);    //size=4,2,1

void RegisterRegHandler(uint32_t addr, SetRegHandler_t set, GetRegHandler_t get);

extern uint8_t g_arrStorRegCache[0x10000];

//we are not considering endians yet, since the target and host cpu both use little endian now.
#define INT_BITS(TYPE, VAL, BITS_START, BITS_SZ)    ( ( (VAL) & ( ( ( (TYPE)1 << (BITS_SZ) ) - 1 ) << (BITS_START) ) ) >> (BITS_START) )
#define REG_BITS(TYPE, ADDR, BITS_START, BITS_SZ)   INT_BITS(TYPE, (*(TYPE*)(g_arrStorRegCache + ADDR)), BITS_START, BITS_SZ)

void Init_DMA();

//base memory management
//1k per block
struct BlockDesc {
    uint8_t *pBase;
    uint8_t bDevReg;
    uint8_t bWritable;
};

struct BlockDesc g_arrBlksDesc[0x10000000 >> 10];

EXP_STATE phym_read8(uint32_t addr, uint8_t *pVal);

EXP_STATE phym_write8(uint32_t addr, uint8_t val);

EXP_STATE phym_read16(uint32_t addr, uint16_t *pVal);

EXP_STATE phym_write16(uint32_t addr, uint16_t val);

EXP_STATE phym_read32(uint32_t addr, uint32_t *pVal);

EXP_STATE phym_write32(uint32_t addr, uint32_t val);

#endif // PHYMEM_H_INCLUDED
