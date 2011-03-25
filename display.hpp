//This is the rendering classes of display system
//only included by display.cpp

class DispLayer
{
public:
    virtual void ReloadCtl(uint16_t usCtlAddr) = 0;
    virtual void StartLine(uint16_t y) = 0;
    virtual uint16_t Render(uint16_t x) = 0;
};

class TiledBg: public DispLayer
{
private:
    uint16_t m_usTileMapBase;
    uint16_t m_usTileBase;
    uint16_t m_usVirSizeX, m_usVirSizeY;
    uint16_t m_usVirTileCntX;
    uint16_t m_usScrollX, m_usScrollY;
    uint16_t m_usCurrLine;
    bool m_b4BitPlt;

public:
    void ReloadCtl(uint16_t usCtlAddr);
    void StartLine(uint16_t y);
    uint16_t Render(uint16_t x);
};

void TiledBg::ReloadCtl(uint16_t usCtlAddr)
{
    m_usVirSizeX = REG_BITS(uint16_t, usCtlAddr, 14, 1) * 256 + 256;
    m_usVirTileCntX = m_usVirSizeX / 8;
    m_usVirSizeX--;
    m_usVirSizeY = REG_BITS(uint16_t, usCtlAddr, 15, 1) * 256 + 256 - 1;

    m_usTileMapBase = REG_BITS(uint16_t, usCtlAddr, 8, 5) * 0x800;
    m_usTileBase = REG_BITS(uint16_t, usCtlAddr, 2, 2) * 0x4000;
    m_b4BitPlt = REG_BITS(uint16_t, usCtlAddr, 7, 1) == 0? true: false;

    m_usScrollX = REG_BITS(uint16_t, (usCtlAddr - 0x0008) * 2 + 0x0010, 0, 9);
    m_usScrollY = REG_BITS(uint16_t, (usCtlAddr - 0x0008) * 2 + 0x0012, 0, 9);
}

void TiledBg::StartLine(uint16_t y)
{
    m_usCurrLine = (m_usScrollY + y) & m_usVirSizeY;
}

uint16_t TiledBg::Render(uint16_t x)
{
    x = ( m_usScrollX + x ) & m_usVirSizeX;
    //y = ( m_usScrollY + y ) & m_usVirSizeY;

    //decide which tile are we in
    uint16_t usTileX = x / 8;
    uint16_t usTileY = m_usCurrLine / 8;
    uint16_t usTileIndex = usTileY * m_usVirTileCntX + usTileX;

    //find the dot palette via the tile index
    uint8_t byDotPalette;
    uint16_t usTileMapEntry = *(uint16_t*)(g_arrStorVram + m_usTileMapBase + 2 * usTileIndex);
    uint16_t usInTileX = x % 8;
    uint16_t usInTileY = m_usCurrLine % 8;
    usInTileX  ^= ( uint16_t(8) - INT_BITS(uint16_t, usTileMapEntry, 10, 1) ) & 0x0007;   //flip horizontally on flag
    usInTileY  ^= ( uint16_t(8) - INT_BITS(uint16_t, usTileMapEntry, 11, 1) ) & 0x0007;   //flip vertical on flag
    //if ( INT_BITS(uint16_t, usTileMapEntry, 10, 1) != 0 ) usInTileX = 7 - usInTileX;
    //if ( INT_BITS(uint16_t, usTileMapEntry, 11, 1) != 0 ) usInTileY = 7 - usInTileY;
    uint16_t usTileDataIndex = INT_BITS(uint16_t, usTileMapEntry, 0, 10);
    if ( m_b4BitPlt ){  //4-bit palette
        byDotPalette = *(g_arrStorVram + m_usTileBase + usTileDataIndex * 0x20 + usInTileY * 4 + usInTileX / 2);
        byDotPalette = ( byDotPalette >> ( usTileX % 2 ) * 4 ) & 0x0F;
        if ( byDotPalette == 0 ) return 0x8000;
        byDotPalette |= uint8_t(( usTileMapEntry & 0xF000 ) >> 8);
    }
    else{
        byDotPalette = *(g_arrStorVram + m_usTileBase + usTileDataIndex * 0x40 + usInTileY * 8 + usInTileX);
        if ( byDotPalette == 0 ) return 0x8000;
    }

    //convert the palette index into rgb
    return g_arrStorPalram[byDotPalette] & 0x7FFF;
}

class TiledBgRS: public DispLayer
{
private:
    uint16_t m_usTileMapBase;
    uint16_t m_usTileBase;
    uint16_t m_usVirSizeXY;
    uint16_t m_usVirTileCntX;
    bool m_b4BitPlt;
    bool m_bOfTrans;

    uint32_t m_ulStartX;
    uint32_t m_ulStartY;
    uint32_t m_ulStartXCurr;
    uint32_t m_ulStartYCurr;
    uint32_t m_ulDx;
    uint32_t m_ulDy;
    uint32_t m_ulDmx;
    uint32_t m_ulDmy;

public:
    void ReloadCtl(uint16_t usCtlAddr);
    void StartLine(uint16_t y);
    uint16_t Render(uint16_t x);
};

void TiledBgRS::ReloadCtl(uint16_t usCtlAddr)
{
    m_usVirSizeXY = uint16_t(128) << REG_BITS(uint16_t, usCtlAddr, 14, 2);
    m_usVirTileCntX = m_usVirSizeXY / 8;
    m_usVirSizeXY--;

    m_usTileMapBase = REG_BITS(uint16_t, usCtlAddr, 8, 5) * 0x800;
    m_usTileBase = REG_BITS(uint16_t, usCtlAddr, 2, 2) * 0x4000;
    m_b4BitPlt = REG_BITS(uint16_t, usCtlAddr, 7, 1) == 0? true: false;
    m_bOfTrans = REG_BITS(uint16_t, usCtlAddr, 13, 1) == 0? true: false;

    m_ulStartX = *(uint32_t*)(g_arrDevRegCache + 0x0028 + (usCtlAddr - 0x000C) * 0x0008);
    m_ulStartX = ( m_ulStartX & 0x0FFFFFFF ) | ((0x0UL - m_ulStartX) & 0x08000000);
    m_ulStartY = *(uint32_t*)(g_arrDevRegCache + 0x002C + (usCtlAddr - 0x000C) * 0x0008);
    m_ulStartY = ( m_ulStartY & 0x0FFFFFFF ) | ((0x0UL - m_ulStartY) & 0x08000000);
    m_ulDx  = (uint32_t)(int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0020 + (usCtlAddr - 0x000C) * 0x0008));
    m_ulDmx = (uint32_t)(int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0022 + (usCtlAddr - 0x000C) * 0x0008));
    m_ulDy  = (uint32_t)(int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0024 + (usCtlAddr - 0x000C) * 0x0008));
    m_ulDmy = (uint32_t)(int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0026 + (usCtlAddr - 0x000C) * 0x0008));
}

void TiledBgRS::StartLine(uint16_t y)
{
    //m_usCurrLine = (m_usScrollY + y) & m_usVirSizeY;
    m_ulStartXCurr = m_ulStartX + m_ulDmx * y;
    m_ulStartYCurr = m_ulStartY + m_ulDmy * y;
}

uint16_t TiledBgRS::Render(uint16_t x)
{
    uint32_t ulX = m_ulStartXCurr + m_ulDx * x;
    uint32_t ulY = m_ulStartYCurr + m_ulDy * x;

    x = uint16_t(ulX >> 8);
    uint16_t y = uint16_t(ulY >> 8);

    if ( m_bOfTrans ){
        if ( x > m_usVirSizeXY || y > m_usVirSizeXY )
            return 0x8000;
    }
    x &= m_usVirSizeXY;
    y &= m_usVirSizeXY;

    //decide which tile are we in
    uint16_t usTileX = x / 8;
    uint16_t usTileY = y / 8;
    uint16_t usTileIndex = usTileY * m_usVirTileCntX + usTileX;

    //find the dot palette via the tile index
    uint8_t byDotPalette;
    uint16_t usTileMapEntry = *(uint16_t*)(g_arrStorVram + m_usTileMapBase + 2 * usTileIndex);
    uint16_t usInTileX = x % 8;
    uint16_t usInTileY = y % 8;
    usInTileX  ^= ( uint16_t(0) - INT_BITS(uint16_t, usTileMapEntry, 10, 1) ) & 0x0007;   //flip horizontally on flag
    usInTileY  ^= ( uint16_t(0) - INT_BITS(uint16_t, usTileMapEntry, 11, 1) ) & 0x0007;   //flip vertical on flag
    //if ( INT_BITS(uint16_t, usTileMapEntry, 10, 1) != 0 ) usInTileX = 7 - usInTileX;
    //if ( INT_BITS(uint16_t, usTileMapEntry, 11, 1) != 0 ) usInTileY = 7 - usInTileY;
    uint16_t usTileDataIndex = INT_BITS(uint16_t, usTileMapEntry, 0, 10);
    if ( m_b4BitPlt ){  //4-bit palette
        byDotPalette = *(g_arrStorVram + m_usTileBase + usTileDataIndex * 0x20 + usInTileY * 4 + usInTileX / 2);
        byDotPalette = ( byDotPalette >> ( usTileX % 2 ) * 4 ) & 0x0F;
        if ( byDotPalette == 0 ) return 0x8000;
        byDotPalette |= uint8_t(( usTileMapEntry & 0xF000 ) >> 8);
    }
    else{
        byDotPalette = *(g_arrStorVram + m_usTileBase + usTileDataIndex * 0x40 + usInTileY * 8 + usInTileX);
        if ( byDotPalette == 0 ) return 0x8000;
    }

    //convert the palette index into rgb
    return g_arrStorPalram[byDotPalette] & 0x7FFF;
}

//objects, later
class ObjRdr
{
private:
    uint16_t buf[64][64];
};

void ReloadDispCtl(uint16_t usDispCtl)
{

}

