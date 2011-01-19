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

static void InitBlockArea(uint32_t ulAeraStart, uint32_t ulAeraSize, uint8_t *pSecBase, uint32_t ulSecSize, uint8_t bRepSec);

void PhyMemInit()
{
    //blocks desc
    InitBlockArea(0, 0x02000000, g_arrStorRom, sizeof(g_arrStorRom), 0);
    InitBlockArea(0x02000000, 0x01000000, g_arrStorRam, sizeof(g_arrStorRam), 1);
    InitBlockArea(0x03000000, 0x01000000, g_arrStorIram, sizeof(g_arrStorIram), 1);
    InitBlockArea(0x04000000, 0x01000000, g_arrStorPalram, sizeof(g_arrStorPalram), 1);
}

//return NULL on mapped registers
inline uint8_t * getmemptr(uint32_t addr)
{
    if ( addr )
}

__fastcall uint8_t phym_read8(uint32_t addr)
{
    if ( )
}
