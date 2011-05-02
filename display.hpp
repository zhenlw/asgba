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
    return g_arrStorPalramBg[byDotPalette] & 0x7FFF;
}

class TiledBgRS: public DispLayer
{
private:
    uint16_t m_usTileMapBase;
    uint16_t m_usTileBase;
    uint16_t m_usVirSizeXY;
    uint16_t m_usVirTileCntX;
    //bool m_b4BitPlt; always 8bit
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
    m_bOfTrans = REG_BITS(uint16_t, usCtlAddr, 13, 1) == 0? true: false;

    m_ulStartX = *(uint32_t*)(g_arrDevRegCache + 0x0028 + (usCtlAddr - 0x000C) * 0x0008);
    m_ulStartX = ( m_ulStartX & 0x0FFFFFFF ) | (0x0UL - (m_ulStartX & 0x08000000));
    m_ulStartY = *(uint32_t*)(g_arrDevRegCache + 0x002C + (usCtlAddr - 0x000C) * 0x0008);
    m_ulStartY = ( m_ulStartY & 0x0FFFFFFF ) | (0x0UL - (m_ulStartY & 0x08000000));
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
    uint16_t usInTileX = x % 8;
    uint16_t usInTileY = y % 8;

	uint16_t usTileDataIndex = *(g_arrStorVram + m_usTileMapBase + usTileIndex);

	uint8_t byDotPalette = *(g_arrStorVram + m_usTileBase + usTileDataIndex * 0x40 + usInTileY * 8 + usInTileX);
    if ( byDotPalette == 0 ) return 0x8000;

    //convert the palette index into rgb
    return g_arrStorPalramBg[byDotPalette] & 0x7FFF;
}

//objects, later
class SprRdr
{
private:
	uint16_t m_usSprCnt;
	struct Spr{
		uint16_t bSR;	//1 = on
		uint16_t bDblSz;	//1 = on
		uint16_t bTransparent;
		uint16_t bObjWindow;
		uint16_t usPrio;

		uint16_t usX, usY;
		uint16_t usSzX, usSzY;	//size in 8 pixel unit (tile)
		uint16_t usSzXReal, usSzYReal;	//pixels of the frame
		uint16_t arrColors[128][128];
	} m_arrSprRendered[128];

	uint16_t m_usCurrLine;
	struct LineBuf{
		uint16_t usSprIdx;
	} m_arrLineBuf[128];
	uint8_t m_bySprsThisLine;
	static uint16_t sizeTable[4][4][2];
	static inline int RenderSpr(uint16_t attr[], Spr &spr);

public:
	void ReloadCtl();
    void StartLine(uint16_t y);
    uint32_t Render(uint16_t x, uint16_t usPrio);	//also return tag for 1st transparent layer
	bool InObjWindow(uint16_t x);
};

uint16_t SprRdr::sizeTable[4][4][2] = {
	{{1,1}, {2,2}, {4,4}, {8,8}},
	{{2,1}, {4,1}, {4,2}, {8,4}},
	{{1,2}, {1,4}, {2,4}, {4,8}}
};

int SprRdr::RenderSpr(uint16_t attr[], Spr &spr)
{
	if ( attr[0] == 0 ) return 1;	//not accurate
	if ( ( attr[0] & 0x0300 ) == 0x0200 ) return 1;

	spr.bObjWindow = 0;
	spr.bTransparent = 0;
	if ( (attr[0] & 0x0C00) == 0x0800 ){	//obj window
		spr.bObjWindow = 1;
	}
	else if ( (attr[0] & 0x0C00) == 0x0400 ){	//tranparent obj
		spr.bTransparent = 1;	//this means 1st layer only
	}
	else if ( (g_arrDevRegCache[0x50] & 0xD0) == 0x50 ){	//transparent effective & obj selected as 1st layer
		spr.bTransparent = 1;	//this means 1st layer only
	}
	
	spr.usPrio = INT_BITS(uint16_t, attr[2], 10, 2);
	spr.usX = INT_BITS(uint16_t, attr[1], 0, 9);
	spr.usX = (uint16_t(0) - (spr.usX & 0x0100)) | spr.usX;
	spr.usY = INT_BITS(uint16_t, attr[0], 0, 8);
	spr.usY = (uint16_t(0) - (spr.usY & 0x0080)) | spr.usY;
	spr.usSzX = sizeTable[INT_BITS(uint16_t, attr[0], 14, 2)][INT_BITS(uint16_t, attr[1], 14, 2)][0];
	spr.usSzY = sizeTable[INT_BITS(uint16_t, attr[0], 14, 2)][INT_BITS(uint16_t, attr[1], 14, 2)][1];
	spr.bSR = INT_BITS(uint16_t, attr[0], 8, 1);
	spr.bDblSz = INT_BITS(uint16_t, attr[0], 9, 1);
	spr.usSzYReal = spr.usSzY << (spr.bDblSz + 3);
	spr.usSzXReal = spr.usSzX << (spr.bDblSz + 3);

	uint16_t usIndex = INT_BITS(uint16_t, attr[2], 0, 10);
	uint8_t byPale = (attr[0] & 0x2000) != 0? 0xFF: (uint8_t(attr[2] >> 8) & 0xF0);

	uint16_t usDx, usDmx, usDy, usDmy;
	if ( spr.bSR ){
		uint16_t us = INT_BITS(uint16_t, attr[1], 9, 5);
		usDx = ((uint16_t *)g_arrStorOam)[us * 4 * 4 + 3];
		usDmx = ((uint16_t *)g_arrStorOam)[us * 4 * 4 + 4 + 3];
		usDy = ((uint16_t *)g_arrStorOam)[us * 4 * 4 + 8 + 3];
		usDmy = ((uint16_t *)g_arrStorOam)[us * 4 * 4 + 12 + 3];
	}
	else{
		usDx = 0x0100;
		usDmx = 0x0000;
		usDy = 0x0000;
		usDmy = 0x0100;
	}
	//calc the start point in spirit data, make sure the center is centered in the real result
	uint16_t usInSprX = ( spr.usSzX << (8 + 3) ) / 2;
	uint16_t usInSprY = ( spr.usSzY << (8 + 3) ) / 2;
	usInSprX -= usDx * ( spr.usSzXReal / 2);
	usInSprY -= usDy * ( spr.usSzXReal / 2);
	usInSprX -= usDmx * ( spr.usSzYReal / 2);
	usInSprY -= usDmy * ( spr.usSzYReal / 2);

	for ( uint16_t iy = 0; iy < spr.usSzYReal; iy++ ){
		uint16_t usInSprXSave = usInSprX;
		uint16_t usInSprYSave = usInSprY;

		for ( uint16_t ix = 0; ix < spr.usSzXReal; ix++ ){
			uint16_t inSprY = int16_t(usInSprY) >> 8;
			uint16_t inSprX = int16_t(usInSprX) >> 8;
			if ( inSprX >= ( spr.usSzX << 3 ) || inSprY >= ( spr.usSzY << 3 ) ){
				spr.arrColors[iy][ix] = 0x8000;
				usInSprX += usDx;
				usInSprY += usDy;
				continue;
			}

			uint16_t yt = inSprY / 8;
			uint16_t yti = inSprY % 8;
			uint16_t xt = inSprX / 8;
			uint16_t xti = inSprX % 8;

			if ( byPale == 0xFF ){
				uint16_t tinx = usIndex + yt * 32 + xt * 2;
				uint16_t intinx = yti * 8 + xti;
				uint8_t byDotPalette = g_arrStorVram[0x10000UL + tinx * 32 + intinx];	//32 means 32 bytes per tile
				if ( byDotPalette != 0 )
					spr.arrColors[iy][ix] = g_arrStorPalramSpr[byDotPalette] & 0x7FFF;
				else
					spr.arrColors[iy][ix] = 0x8000;
			}
			else{
				uint16_t tinx = usIndex + yt * 32 + xt;	//32 means 32 tiles per hori line
				uint16_t intinx = yti * 8 + xti;
				uint8_t byDotPalette = g_arrStorVram[0x10000UL + tinx * 32 + intinx / 2];	//32 means 32 bytes per tile
		        byDotPalette = ( byDotPalette >> ( intinx % 2 ) * 4 ) & 0x0F;
				if ( byDotPalette != 0 )
					spr.arrColors[iy][ix] = g_arrStorPalramSpr[byDotPalette|byPale] & 0x7FFF;
				else
					spr.arrColors[iy][ix] = 0x8000;
			}
			usInSprX += usDx;
			usInSprY += usDy;
		}
		usInSprX = usInSprXSave + usDmx;
		usInSprY = usInSprYSave + usDmy;
	}
	return 0;
}

void SprRdr::ReloadCtl()
{
	m_usSprCnt = 0;
	if ( (g_arrDevRegCache[1] & 0x10) == 0 ){	//check reg 0000
		return;
	}

	//go through the oam list
	uint16_t *pSprt = (uint16_t *)g_arrStorOam;
	for ( int i = 0; i < 128; i++ ){
		int j = RenderSpr(pSprt, m_arrSprRendered[m_usSprCnt]);
		if ( j < 0 ) return;
		if ( j == 0 )
			m_usSprCnt++;
		pSprt += 4;
	}
}

void SprRdr::StartLine(uint16_t y)
{
    m_usCurrLine = y;
	m_bySprsThisLine = 0;
	//go through the rendered spirits
	for ( uint16_t i = 0; i < m_usSprCnt; i++ ){
		Spr *pSpr = m_arrSprRendered + i;
		if ( uint16_t(y - pSpr->usY) < pSpr->usSzYReal ){	//usY is actually signed
			m_arrLineBuf[m_bySprsThisLine++].usSprIdx = i;
		}
	}
}

uint32_t SprRdr::Render(uint16_t x, uint16_t usPrio)
{
	for ( uint8_t i = 0; i < m_bySprsThisLine; i++ ){
		Spr *pSpr = m_arrSprRendered + m_arrLineBuf[i].usSprIdx;
		if ( pSpr->usPrio == usPrio && pSpr->bObjWindow == 0 ){
			if ( uint16_t(x - pSpr->usX) < pSpr->usSzXReal ){
				uint16_t us = pSpr->arrColors[uint16_t(m_usCurrLine - pSpr->usY)][uint16_t(x - pSpr->usX)];
				if ( us != 0x8000 )
					return uint32_t(us) | (uint32_t(pSpr->bTransparent) << 16);
			}
		}
	}
	return 0x8000UL;
}

bool SprRdr::InObjWindow(uint16_t x)
{
	for ( uint8_t i = 0; i < m_bySprsThisLine; i++ ){
		Spr *pSpr = m_arrSprRendered + m_arrLineBuf[i].usSprIdx;
		if ( pSpr->bObjWindow != 0 ){
			if ( uint16_t(x - pSpr->usX) < pSpr->usSzXReal ){
				uint16_t us = pSpr->arrColors[uint16_t(m_usCurrLine - pSpr->usY)][uint16_t(x - pSpr->usX)];
				if ( us != 0x8000 )
					return true;
			}
		}
	}
	return false;
}
