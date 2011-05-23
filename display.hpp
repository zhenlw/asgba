#include "gui/GLee.h"

//the rendering classes of display system

class DispContext
{
public:
	//data
	static bool m_bForceBlank;
	static bool m_bSpr2DTiles;
	static uint8_t *m_pSprVramBase;
	static uint8_t m_arrColorBD[4];
	static uint32_t m_ulCurrFrameCnt;

	static void ReloadCtl();
	static void Init();
};

bool DispContext::m_bForceBlank;
bool DispContext::m_bSpr2DTiles;
uint8_t *DispContext::m_pSprVramBase;
uint8_t DispContext::m_arrColorBD[4];
uint32_t DispContext::m_ulCurrFrameCnt;

inline void FILL_COLOR(uint8_t *pDot, uint16_t us)
{
	pDot[0] = uint8_t( us << 3 );
	pDot[0] = pDot[0] | (pDot[0] >> 5);
	pDot[1] = uint8_t( us >> 2 ) & 0xF8;
	pDot[1] = pDot[1] | (pDot[1] >> 5);
	pDot[2] = uint8_t( us >> 7 ) & 0xF8;
	pDot[2] = pDot[2] | (pDot[2] >> 5);
}


struct TextureLoader
{
	uint16_t usTxtIndex;
	uint16_t usTxtrWidth, usTxtrHeight;
	uint8_t *pCache;	//should be NULL after loaded?

private:
	uint16_t usTileBytes;
	uint8_t byPalet;
	uint8_t *pTgt, *pSrc;
	const uint16_t *pPalRam;
	inline void LoadTile(){
		int i, j;
		uint8_t val;
		if ( usTileBytes == 64 ){
			for ( i = 0; i < 8; i++ ){
				for ( j = 0; j < 8; j++ ){
					if ( *pSrc == 0 ) pTgt[3] = 0;
					else{
						FILL_COLOR(pTgt, pPalRam[*pSrc]);
						pTgt[3] = 1;
					}
					pTgt += 4;
					pSrc++;
				}
				pTgt += (usTxtrWidth - 8) * 4;
			}
		}
		else{
			for ( i = 0; i < 8; i++ ){
				for ( j = 0; j < 4; j++ ){
					//first the lower 4b
					val = (*pSrc) & 0x000F;
					if ( val == 0 ) pTgt[3] = 0;
					else{
						FILL_COLOR(pTgt, pPalRam[val|byPalet]);
						pTgt[3] = 1;
					}
					pTgt += 4;
					val = (*pSrc) >> 4;
					if ( val == 0 ) pTgt[3] = 0;
					else{
						FILL_COLOR(pTgt, pPalRam[val|byPalet]);
						pTgt[3] = 1;
					}
					pTgt += 4;
					pSrc++;
				}
				pTgt += (usTxtrWidth - 8) * 4;
			}
		}
	};
public:
	void LoadTiledRS(uint8_t *tileIndex, uint8_t *tileBase){
		pPalRam = g_arrStorPalramBg;
		pTgt = pCache;
		usTileBytes = 64;
		for ( int i = 0; i < usTxtrHeight / 8; i++ ){
			for ( int j = 0; j < usTxtrWidth / 8; j++ ){
				pSrc = tileBase + (*tileIndex) * 64;
				LoadTile();
				pTgt = pTgt - usTxtrWidth * 8 * 4 + 8 * 4;
				tileIndex++;
			}
			pTgt = pTgt + usTxtrWidth * 7 * 4;
		}
	};
	void LoadTiled(uint16_t *tileIndex, uint8_t *tileBase, uint16_t tileBytes){	//tiled bg
		pPalRam = g_arrStorPalramBg;
		pTgt = pCache;
		usTileBytes = tileBytes;
		for ( int i = 0; i < 32; i++ ){
			for ( int j = 0; j < 32; j++ ){
				byPalet = uint8_t((*tileIndex >> 8) & 0x00F0);
				pSrc = tileBase + ((*tileIndex) & 0x03FF) * usTileBytes;
				LoadTile();
				pTgt = pTgt - usTxtrWidth * 8 * 4 + 8 * 4;
				tileIndex++;
			}
			if ( usTxtrWidth > 256 ){
				tileIndex += 32 * 32 - 32;
				for ( int j = 0; j < 32; j++ ){
					byPalet = uint8_t((*tileIndex >> 8) & 0x00F0);
					pSrc = tileBase + ((*tileIndex) & 0x03FF) * usTileBytes;
					LoadTile();
					pTgt = pTgt - usTxtrWidth * 8 * 4 + 8 * 4;
					tileIndex++;
				}
				tileIndex -= 32 * 32;
			}
			pTgt = pTgt + usTxtrWidth * 7 * 4;
		}
		if ( usTxtrHeight > 256 ) tileIndex += 32 * 32;
		for ( int i = 0; i < 32; i++ ){
			for ( int j = 0; j < 32; j++ ){
				byPalet = uint8_t((*tileIndex >> 8) & 0x00F0);
				pSrc = tileBase + ((*tileIndex) & 0x03FF) * usTileBytes;
				LoadTile();
				pTgt = pTgt - usTxtrWidth * 8 * 4 + 8 * 4;
				tileIndex++;
			}
			if ( usTxtrWidth > 256 ){
				tileIndex += 32 * 32 - 32;
				for ( int j = 0; j < 32; j++ ){
					byPalet = uint8_t((*tileIndex >> 8) & 0x00F0);
					pSrc = tileBase + ((*tileIndex) & 0x03FF) * usTileBytes;
					LoadTile();
					pTgt = pTgt - usTxtrWidth * 8 * 4 + 8 * 4;
					tileIndex++;
				}
				tileIndex -= 32 * 32;
			}
			pTgt = pTgt + usTxtrWidth * 7 * 4;
		}

	};
	void LoadBmp(uint16_t mode, uint16_t frame2){
		pPalRam = g_arrStorPalramBg;
		int32_t iDotCnt = 240 * 160;
		if ( mode == 5 )
			iDotCnt = 160 * 128;
		pTgt = pCache;
		if ( mode == 4 ){
			uint8_t *src = g_arrStorVram + uint32_t(frame2) * 0x0A000;
			for ( int32_t i = iDotCnt; i > 0; i-- ){
				if ( *src == 0 ) pTgt[3] = 0;
				else{
					FILL_COLOR(pTgt, g_arrStorPalramBg[*src]);
					pTgt[3] = 1;
				}
				pTgt += 4;
				src++;
			}
		}
		else{
			uint16_t *src = (uint16_t*)(g_arrStorVram + uint32_t(frame2) * 0x0A000);
			for ( int32_t i = iDotCnt; i > 0; i-- ){
				if ( *src == 0 ) pTgt[3] = 0;	//not sure about this
				else{
					FILL_COLOR(pTgt, *src);
					pTgt[3] = 1;
				}
				pTgt += 4;
				src++;
			}
		}
	};
	void LoadSpr(uint8_t *tileBase, byte palet){
		pPalRam = g_arrStorPalramSpr;
		pTgt = pCache;
		pSrc = tileBase;
		uint16_t usShiftPerTileLine = 0;
		uint16_t szX = usTxtrWidth / 8;
		if ( palet == 0xFF ) usTileBytes = 64;
		else usTileBytes = 32;
		if ( DispContext::m_bSpr2DTiles ) usShiftPerTileLine = 32 * 32 - usTileBytes * szX;
		byPalet = palet;
		for ( int i = usTxtrHeight / 8; i != 0; i-- ){
			for ( int j = szX; j != 0; j-- ){
				LoadTile();
				pTgt = pTgt - usTxtrWidth * 8 * 4 + 8 * 4;
			}
			pSrc += usShiftPerTileLine;
			pTgt = pTgt + usTxtrWidth * 7 * 4;
		}
	};
};

struct TextureRdrer
{
	int32_t lTxtrCoorX, lTxtrCoorY;	//fix point with 8-bit fragments, and signed
	int32_t lDx, lDy, lDmx, lDmy;
	uint16_t usRealDispWidth, usRealDispHeight;
	int16_t sRealStartX, sRealStartY;

	inline void Draw(TextureLoader &loader)
	{
		int32_t lx = lTxtrCoorX, ly = lTxtrCoorY;
		float fx1 = float(lx) / float(loader.usTxtrWidth << 8);
		float fy1 = float(ly) / float(loader.usTxtrHeight << 8);
		lx += lDx * usRealDispWidth;
		ly += lDy * usRealDispWidth;
		float fx2 = float(lx) / float(loader.usTxtrWidth << 8);
		float fy2 = float(ly) / float(loader.usTxtrHeight << 8);
		lx += lDmx * usRealDispHeight;
		ly += lDmy * usRealDispHeight;
		float fx3 = float(lx) / float(loader.usTxtrWidth << 8);
		float fy3 = float(ly) / float(loader.usTxtrHeight << 8);
		lx -= lDx * usRealDispWidth;
		ly -= lDy * usRealDispWidth;
		float fx4 = float(lx) / float(loader.usTxtrWidth << 8);
		float fy4 = float(ly) / float(loader.usTxtrHeight << 8);

		glBindTexture(GL_TEXTURE_2D, loader.usTxtIndex);

		if ( loader.pCache != NULL ){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_ARB);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_ARB);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, loader.usTxtrWidth, loader.usTxtrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, loader.pCache);

			loader.pCache = NULL;
		}

		glBegin(GL_QUADS);
		glTexCoord2f(fx1, fy1);
		glVertex2s(sRealStartX, 160 - sRealStartY);
		glTexCoord2f(fx2, fy2);
		glVertex2s(sRealStartX + usRealDispWidth, 160 - sRealStartY);
		glTexCoord2f(fx3, fy3);
		glVertex2s(sRealStartX + usRealDispWidth, 160 - sRealStartY - usRealDispHeight);
		glTexCoord2f(fx4, fy4);
		glVertex2s(sRealStartX, 160 - sRealStartY - usRealDispHeight);
		glEnd();
	}
};

//objects
class SprRdr
{
private:
	static uint16_t m_sizeTable[4][4][2];

	struct SprData{
		uint16_t usPrio;
		uint16_t usTxtrKey;
		uint8_t arrTxtrCache[64*64*4];
		TextureLoader txtrLdr;
		TextureRdrer txtrDrwr;
		bool bObjWnd;
	};

	SprData m_arrSprs[128];
	uint8_t m_arrSprMap[16];	//load temp usage only
	bool m_bObjWnd;

	inline void LoadSpr(int idx){
		uint16_t *attr = ((uint16_t *)g_arrStorOam) + idx * 4;
		if ( attr[0] == 0 ) return;	//not accurate
		if ( ( attr[0] & 0x0300 ) == 0x0200 ) return;

		m_arrSprMap[idx/8] |= (1 << (idx % 8));
		SprData *p = m_arrSprs + idx;
		if ( (attr[0] & 0x0C00) == 0x0800 ){	//obj window
			p->bObjWnd = true;
		}
		else{
			p->bObjWnd = false;
		}
		/*else if ( (attr[0] & 0x0C00) == 0x0400 ){	//tranparent obj
			bTransparent = 1;	//this means 1st layer only
		}
		else if ( (g_arrDevRegCache[0x50] & 0xD0) == 0x50 ){	//transparent effective & obj selected as 1st layer
			bTransparent = 1;	//this means 1st layer only
		}*/

		//combine original size with index, to determine if the texture should be loaded
		uint16_t usIndex = INT_BITS(uint16_t, attr[2], 0, 10);
		uint16_t usShape = INT_BITS(uint16_t, attr[0], 14, 2);
		uint16_t usSzIdx = INT_BITS(uint16_t, attr[1], 14, 2);
		uint16_t usKey = (usShape << 12) | (usSzIdx << 10) | usIndex;

		//go through current loaded textures
		/*for ( int i = 0; i < 16; i++ ){
			if ( m_arrSprMap[i] != 0 ){
				for (int j = 0; j < 8; j++ ){
					//if ( m_arrSpr)
				}
			}
		}*/
		uint16_t usSzX = m_sizeTable[usShape][usSzIdx][0];
		uint16_t usSzY = m_sizeTable[usShape][usSzIdx][1];

		p->txtrLdr.pCache = p->arrTxtrCache;
		p->txtrLdr.usTxtIndex = idx;
		p->txtrLdr.usTxtrHeight = usSzY * 8;
		p->txtrLdr.usTxtrWidth = usSzX * 8;
		p->txtrLdr.LoadSpr(DispContext::m_pSprVramBase + usIndex * 32, (attr[0] & 0x2000) != 0? 0xFF: (uint8_t(attr[2] >> 8) & 0xF0));

		p->usTxtrKey = usKey;
		p->usPrio = INT_BITS(uint16_t, attr[2], 10, 2);

		TextureRdrer &txtr = p->txtrDrwr;
		txtr.sRealStartX = int16_t(attr[1] << 7) >> 7;
		txtr.sRealStartY = int16_t(attr[0] << 8) >> 8;

		uint16_t bSR = INT_BITS(uint16_t, attr[0], 8, 1);
		uint16_t bDblSz = INT_BITS(uint16_t, attr[0], 9, 1);
		txtr.usRealDispHeight = p->txtrLdr.usTxtrHeight << bDblSz;
		txtr.usRealDispWidth = p->txtrLdr.usTxtrWidth << bDblSz;

		if ( bSR ){
			uint16_t us = INT_BITS(uint16_t, attr[1], 9, 5);
			txtr.lDx = int32_t(((int16_t *)g_arrStorOam)[us * 4 * 4 + 3]);
			txtr.lDmx = int32_t(((int16_t *)g_arrStorOam)[us * 4 * 4 + 4 + 3]);
			txtr.lDy = int32_t(((int16_t *)g_arrStorOam)[us * 4 * 4 + 8 + 3]);
			txtr.lDmy = int32_t(((int16_t *)g_arrStorOam)[us * 4 * 4 + 12 + 3]);
		}
		else{
			txtr.lDx = 0x0100;
			txtr.lDmx = 0x0000;
			txtr.lDy = 0x0000;
			txtr.lDmy = 0x0100;
		}
		//calc the start point in spirit data, make sure the center is centered in the real result
		txtr.lTxtrCoorX = ( usSzX << (8 + 3) ) / 2;
		txtr.lTxtrCoorY = ( usSzY << (8 + 3) ) / 2;
		txtr.lTxtrCoorX -= txtr.lDx * ( txtr.usRealDispWidth / 2);
		txtr.lTxtrCoorY -= txtr.lDy * ( txtr.usRealDispWidth / 2);
		txtr.lTxtrCoorX -= txtr.lDmx * ( txtr.usRealDispHeight / 2);
		txtr.lTxtrCoorY -= txtr.lDmy * ( txtr.usRealDispHeight / 2);
	};

public:

	void Init(){
		//m_mapSprTxtr.clear();
	};
	void ReloadCtl(){
		memset(m_arrSprMap, 0, 16);
		m_bObjWnd = false;

		if ( (g_arrDevRegCache[1] & 0x10) == 0 ){	//check obj flag
			return;
		}
		if ( (g_arrDevRegCache[1] & 0x80) != 0 ){	//obj windows subject to obj flag, could be wrong
			m_bObjWnd = true;
		}
		//go through the oam list
		//uint16_t *pSprt = (uint16_t *)g_arrStorOam;
		for ( int i = 0; i < 128; i++ ){
			m_arrSprs[i].usPrio = 0xFFFF;
			LoadSpr(i);
		}
	};
	void RenderToGl(uint16_t usPrio){	//called in OpenGL thread
		//normal spirits
		for ( int i = 127; i >= 0; i-- ){
			if ( ( m_arrSprs[i].usPrio == usPrio ) && !m_arrSprs[i].bObjWnd )
				m_arrSprs[i].txtrDrwr.Draw(m_arrSprs[i].txtrLdr);
		}
	};
	void ObjWndToStencil(){
		if ( !m_bObjWnd ){
			return;
		}

		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 1, 1);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
		glDrawBuffer(GL_AUX0);

		for ( int i = 127; i >= 0; i-- ){
			if ( ( m_arrSprs[i].usPrio != 0xFFFF ) && m_arrSprs[i].bObjWnd )
				m_arrSprs[i].txtrDrwr.Draw(m_arrSprs[i].txtrLdr);
		}

		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glDrawBuffer(GL_BACK);
	};
	bool ApplyStencil(uint8_t byLayer){
		if ( !m_bObjWnd ){
			glDisable(GL_STENCIL_TEST);
			return true;
		}
		if ( (byLayer & g_arrDevRegCache[0x004B]) == 0 ){	//masked inside
			if ( (byLayer & g_arrDevRegCache[0x004A]) == 0 ) return false;	//masked both ways
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_EQUAL, 0, 1);
		}
		else if ( (byLayer & g_arrDevRegCache[0x004A]) == 0 ){
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_LESS, 0, 1);
		}
		else{
			glDisable(GL_STENCIL_TEST);
		}
		return true;
	};
};

uint16_t SprRdr::m_sizeTable[4][4][2] = {
	{{1,1}, {2,2}, {4,4}, {8,8}},
	{{2,1}, {4,1}, {4,2}, {8,4}},
	{{1,2}, {1,4}, {2,4}, {4,8}}
};

SprRdr g_SprRdr;

class BgRdr
{
private:
	struct BgData{
		uint16_t usPrio;
		TextureLoader txtrLdr;
		TextureRdrer txtr;
		uint8_t arrTxtr[1024*1024*4];
	} m_arrBgs[4];

	inline void LoadBgRs(uint16_t usNum)
	{
		BgData *p = m_arrBgs + usNum;
		uint16_t usCtlAddr = 0x0008 + usNum * 2;	//for R/S bgs, usNum is alwasy 2 or 3
		if ( (g_arrDevRegCache[1] & (1 << usNum)) != 0 )
			p->usPrio = REG_BITS(uint16_t, usCtlAddr, 0, 2);
		else
			return;
		uint16_t usSzType = REG_BITS(uint16_t, usCtlAddr, 14, 2);
		p->txtrLdr.usTxtrHeight = uint16_t(128) << usSzType;
		p->txtrLdr.usTxtrWidth = p->txtrLdr.usTxtrHeight;
		p->txtrLdr.pCache = p->arrTxtr;
		p->txtrLdr.usTxtIndex = 129 + usNum;

		uint16_t usTileMapAddr = REG_BITS(uint16_t, usCtlAddr, 8, 5) * 0x800;
		uint16_t usTileBase = REG_BITS(uint16_t, usCtlAddr, 2, 2) * 0x4000;
		p->txtrLdr.LoadTiledRS(g_arrStorVram + usTileMapAddr, g_arrStorVram + usTileBase);

		//m_bOfTrans = REG_BITS(uint16_t, usCtlAddr, 13, 1) == 0? true: false;

		p->txtr.lTxtrCoorX = *(int32_t*)(g_arrDevRegCache + 0x0028 + (usCtlAddr - 0x000C) * 0x0008);
		p->txtr.lTxtrCoorX = (p->txtr.lTxtrCoorX << 4) >> 4;
		p->txtr.lTxtrCoorY = *(int32_t*)(g_arrDevRegCache + 0x002C + (usCtlAddr - 0x000C) * 0x0008);
		p->txtr.lTxtrCoorY = (p->txtr.lTxtrCoorY << 4) >> 4;

		p->txtr.lDx  = (int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0020 + (usCtlAddr - 0x000C) * 0x0008));
		p->txtr.lDmx = (int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0022 + (usCtlAddr - 0x000C) * 0x0008));
		p->txtr.lDy  = (int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0024 + (usCtlAddr - 0x000C) * 0x0008));
		p->txtr.lDmy = (int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0026 + (usCtlAddr - 0x000C) * 0x0008));

		//if ( p->cvterBgRs.Refresh(usSzType, g_arrStorVram + usTileMapAddr, g_arrStorVram + usTileBase, p->arrTxtr) )

		p->txtr.sRealStartX = 0;
		p->txtr.sRealStartY = 0;
		p->txtr.usRealDispWidth = 240;
		p->txtr.usRealDispHeight = 160;
	};

	inline void LoadBgBmp(uint16_t usMode)	//must be bg2
	{
		BgData *p = m_arrBgs + 2;
		//uint16_t usCtlAddr = 0x0008 + usNum * 2;	//for R/S bgs, usNum is alwasy 2 or 3
		if ( (g_arrDevRegCache[1] & (1 << 2)) != 0 )
			p->usPrio = REG_BITS(uint16_t, 0x000C, 0, 2);
		else
			return;

		if ( usMode == 5 ){
			p->txtrLdr.usTxtrHeight = 128;
			p->txtrLdr.usTxtrWidth = 160;
		}
		else{
			if ( usMode == 4 )
			p->txtrLdr.usTxtrHeight = 160;
			p->txtrLdr.usTxtrWidth = 240;
		}
		p->txtrLdr.pCache = p->arrTxtr;
		p->txtrLdr.usTxtIndex = 129 + 2;
		p->txtrLdr.LoadBmp(usMode, REG_BITS(uint16_t, 0, 4, 1));

		p->txtr.lTxtrCoorX = *(int32_t*)(g_arrDevRegCache + 0x0028);
		p->txtr.lTxtrCoorX = (p->txtr.lTxtrCoorX << 4) >> 4;
		p->txtr.lTxtrCoorY = *(int32_t*)(g_arrDevRegCache + 0x002C);
		p->txtr.lTxtrCoorY = (p->txtr.lTxtrCoorY << 4) >> 4;

		p->txtr.lDx  = (int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0020));
		p->txtr.lDmx = (int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0022));
		p->txtr.lDy  = (int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0024));
		p->txtr.lDmy = (int32_t)(*(int16_t*)(g_arrDevRegCache + 0x0026));


		p->txtr.sRealStartX = 0;
		p->txtr.sRealStartY = 0;
		p->txtr.usRealDispWidth = 240;
		p->txtr.usRealDispHeight = 160;
	};

	inline void LoadBgTiled(uint16_t usNum)
	{
		BgData *p = m_arrBgs + usNum;
		uint16_t usCtlAddr = 0x0008 + usNum * 2;
		if ( (g_arrDevRegCache[1] & (1 << usNum)) != 0 )
			p->usPrio = REG_BITS(uint16_t, usCtlAddr, 0, 2);
		else
			return;
		uint16_t usSzType = REG_BITS(uint16_t, usCtlAddr, 14, 2);
		p->txtrLdr.usTxtrHeight = uint16_t(256) << (usSzType >> 1);
		p->txtrLdr.usTxtrWidth = uint16_t(256) << (usSzType & 0x01);
		p->txtrLdr.pCache = p->arrTxtr;
		p->txtrLdr.usTxtIndex = 129 + usNum;

		uint16_t usTileMapAddr = REG_BITS(uint16_t, usCtlAddr, 8, 5) * 0x800;
		uint16_t usTileBase = REG_BITS(uint16_t, usCtlAddr, 2, 2) * 0x4000;
		p->txtrLdr.LoadTiled((uint16_t*)(g_arrStorVram + usTileMapAddr), g_arrStorVram + usTileBase, 32 << REG_BITS(uint16_t, usCtlAddr, 7, 1));

		p->txtr.lTxtrCoorX = REG_BITS(uint16_t, 0x10 + 4 * usNum, 0, 9);
		p->txtr.lTxtrCoorX = ( (p->txtr.lTxtrCoorX << 23) >> 15 );
		p->txtr.lTxtrCoorY = REG_BITS(uint16_t, 0x12 + 4 * usNum, 0, 9);
		p->txtr.lTxtrCoorY = ( (p->txtr.lTxtrCoorY << 23) >> 15 );

		p->txtr.lDx  = 0x0100;
		p->txtr.lDmx = 0;
		p->txtr.lDy  = 0;
		p->txtr.lDmy = 0x0100;

		p->txtr.sRealStartX = 0;
		p->txtr.sRealStartY = 0;
		p->txtr.usRealDispWidth = 240;
		p->txtr.usRealDispHeight = 160;
	};

public:

	void Init(){
		
	};
	void ReloadCtl(){
		for ( int i = 0; i < 4; i++ )
			m_arrBgs[i].usPrio = 0xFFFF;
		switch ( REG_BITS(uint16_t, 0, 0, 3) ){
		case 0:
			LoadBgTiled(0);
			LoadBgTiled(1);
			LoadBgTiled(2);
			LoadBgTiled(3);
			break;
		case 1:
			LoadBgTiled(0);
			LoadBgTiled(1);
			LoadBgRs(2);
			break;
		case 2:
			LoadBgRs(2);
			LoadBgRs(3);
			break;
		case 3:
			LoadBgBmp(3);
			break;
		case 4:
			LoadBgBmp(4);
			break;
		case 5:
			LoadBgBmp(5);
			break;
		default:
			throw "display mode not supported";
		}
	};
	void RenderToGl(uint16_t usPrio){
		for ( int i = 3; i >= 0; i-- ){
			if ( m_arrBgs[i].usPrio == usPrio ){
				g_SprRdr.ApplyStencil(1 << i);
				m_arrBgs[i].txtr.Draw(m_arrBgs[i].txtrLdr);
			}
		}
	};
};

BgRdr g_BgRdr;

void DispContext::ReloadCtl()
{
	m_ulCurrFrameCnt++;
	FILL_COLOR(m_arrColorBD, g_arrStorPalramBg[0]);
	m_arrColorBD[3] = 0;

	if ( REG_BITS(uint16_t, 0, 7, 1) != 0 ){    //force blank
		m_bForceBlank = true;
		return;
	}
	m_bForceBlank = false;

	if ( g_arrDevRegCache[0] & 0x40 )
		m_bSpr2DTiles = false;
	else
		m_bSpr2DTiles = true;

	if ( REG_BITS(uint16_t, 0, 0, 3) > 2 )
		m_pSprVramBase = g_arrStorVram + 80 * 1024;
	else
		m_pSprVramBase = g_arrStorVram + 64 * 1024;

	g_SprRdr.ReloadCtl();
	g_BgRdr.ReloadCtl();
}

void DispContext::Init()
{
	g_SprRdr.Init();
	g_BgRdr.Init();
	m_ulCurrFrameCnt = 0xFFFFFFFF;
}

void DispCtxToGl()
{
	glClearColor(float(DispContext::m_arrColorBD[0])/255.0, float(DispContext::m_arrColorBD[1])/255.0, float(DispContext::m_arrColorBD[2])/255.0, 0.0);
	//glClearColor(0, 0, 0, 0.0);
	//glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	//glDrawBuffer(GL_BACK);
	if ( DispContext::m_bForceBlank ) return;

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);

	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	//prepare the stencil
	g_SprRdr.ObjWndToStencil();

	for ( uint16_t i = 3; int16_t(i) >= 0 ; i-- ){
		g_BgRdr.RenderToGl(i);
		if ( g_SprRdr.ApplyStencil(0x10) )
			g_SprRdr.RenderToGl(i);
	}

	glDisable(GL_TEXTURE_2D);

	g_SprRdr.Init();

}
