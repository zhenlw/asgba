#include "phymem.h"

uint16_t TiledBg(uint32_t ulBgCntAddr, uint16_t x, uint16_t y)
{
    uint16_t usVirSizeX = REG_BITS(uint16_t, ulBgCntAddr, 14, 1) * 256 + 256;
    uint16_t usVirSizeY = REG_BITS(uint16_t, ulBgCntAddr, 15, 1) * 256 + 256;
    x = x % usVirSizeX;
    y = y % usVirSizeY;

    //decide which tile are we in
    uint16_t usTileX = x / 8;
    uint16_t usTileY = y / 8;
    uint16_t usTileIndex = usTileY * ( usVirSizeX / 8 ) + usTileX;

    //find the dot palette via the tile index
    uint8_t byDotPalette;
    uint16_t usTileMapEntry = *(uint16_t*)(g_arrStorVram + REG_BITS(uint16_t, ulBgCntAddr, 8, 5) * 0x800 + 2 * usTileIndex);
    uint16_t usInTileX = x % 8;
    uint16_t usInTileY = y % 8;
    if ( INT_BITS(uint16_t, usTileMapEntry, 10, 1) != 0 ) usInTileX = 7 - usInTileX;
    if ( INT_BITS(uint16_t, usTileMapEntry, 11, 1) != 0 ) usInTileY = 7 - usInTileY;
    uint16_t usTileDataIndex = INT_BITS(uint16_t, usTileMapEntry, 0, 10);
    if ( REG_BITS(uint16_t, ulBgCntAddr, 7, 1) == 0 ){  //4-bit palette
        byDotPalette = *(g_arrStorVram + REG_BITS(uint16_t, ulBgCntAddr, 2, 2) * 0x4000 + usTileDataIndex * 0x20 + usInTileY * 4 + usInTileX / 2);
        byDotPalette = ( byDotPalette >> ( usTileX % 2 ) * 4 ) & 0x0F;
        if ( byDotPalette == 0 ) return 0x8000;
        byDotPalette |= uint8_t(( usTileMapEntry & 0xF000 ) >> 8);
    }
    else{
        byDotPalette = *(g_arrStorVram + REG_BITS(uint16_t, ulBgCntAddr, 2, 2) * 0x4000 + usTileDataIndex * 0x40 + usInTileY * 8 + usInTileX);
        if ( byDotPalette == 0 ) return 0x8000;
    }

    //convert the palette index into rgb
    return g_arrStorPalram[byDotPalette] & 0x7FFF;
}

void Init_Display()
{

}
