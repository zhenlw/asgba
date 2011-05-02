#include "phymem.h"
#include "cpu.h"
#include "interrupt.h"

static uint16_t s_usTicksH;
static uint16_t s_usTicksHNextEvt;
static uint16_t s_usCurrLine;

static uint8_t s_byVBlankOn;
static uint8_t s_byHBlankOn;
static uint8_t s_byVCountOn;
static uint8_t s_byVBlankIntr;
static uint8_t s_byHBlankIntr;
static uint8_t s_byVCountIntr;

#include "display.hpp"

static DispLayer *s_arrObjLayers[4] = {NULL, NULL, NULL, NULL};    //indexed by priority
static DispLayer *s_arrBgs[4] = {NULL, NULL, NULL, NULL};  //indexed by bg slot
static uint16_t s_arrBgsPrio[4];     //the priorities among bgs and objs is not clear
static bool s_bForceBlank;
static SprRdr s_SprRdr;

static uint8_t s_BD[3];
static uint16_t s_eva, s_evb;
static uint16_t s_bObjWindowOn;

uint8_t *g_arrRndrBuf = NULL;

static inline void FILL_COLOR(uint8_t *pDot, uint16_t us)
{
	pDot[0] = uint8_t( us << 3 );
	pDot[0] = pDot[0] | (pDot[0] >> 5);
	pDot[1] = uint8_t( us >> 2 ) & 0xF8;
	pDot[1] = pDot[1] | (pDot[1] >> 5);
	pDot[2] = uint8_t( us >> 7 ) & 0xF8;
	pDot[2] = pDot[2] | (pDot[2] >> 5);
}

static inline void FILL_COLOR_BLENDING(uint8_t *pDot, uint16_t us1, uint16_t us2)
{
	uint8_t temp[3];
	FILL_COLOR(temp, us1);
	FILL_COLOR(pDot, us2);

	pDot[0] = ((uint16_t(temp[0]) * s_eva) >> 4) + ((uint16_t(pDot[0]) * s_evb) >> 4);
	pDot[1] = ((uint16_t(temp[1]) * s_eva) >> 4) + ((uint16_t(pDot[1]) * s_evb) >> 4);
	pDot[2] = ((uint16_t(temp[2]) * s_eva) >> 4) + ((uint16_t(pDot[2]) * s_evb) >> 4);
}

void DisplayRenderLine()
{
    uint8_t *pDot = g_arrRndrBuf + uint32_t(s_usCurrLine) * 240 * 3;
	uint16_t us;
	uint32_t ul;

	if ( s_bForceBlank ) return;

	s_SprRdr.StartLine(s_usCurrLine);
    for ( uint16_t i = 0; i < 4; i++ ){
        if ( s_arrBgsPrio[i] != 0xFFFF )
            s_arrBgs[i]->StartLine(s_usCurrLine);
    }
    for ( uint16_t x = 0; x < 240; x++ ){
		pDot[0] = s_BD[0];
		pDot[1] = s_BD[1];
		pDot[2] = s_BD[2];

		//windows processing
		uint8_t byLayerMask = 0xFF;
		if ( s_bObjWindowOn != 0 ){
			if ( s_SprRdr.InObjWindow(x) )
				byLayerMask = g_arrDevRegCache[0x4B];
			else
				byLayerMask = g_arrDevRegCache[0x4A];
		}

		bool bTranspOn = false;
		uint16_t usSavedLayer1;

		//render
        for ( uint16_t prio = 0; prio < 4; prio++ ){
			if ( (byLayerMask & 0x10) != 0 ){
				//spirits of this priority
				ul = s_SprRdr.Render(x, prio);
				if ( ul != 0x8000UL ){
					if ( bTranspOn ){
						if ( (g_arrDevRegCache[0x51] & 0x10) != 0 ){	//second layer found
							FILL_COLOR_BLENDING(pDot, usSavedLayer1, uint16_t(ul));
						}
						else{	//just fill with the saved color
							FILL_COLOR(pDot, usSavedLayer1);
						}
						bTranspOn = false;
						break;
					}
					else{
						if ( ul > 0x8000UL ){	//render it as the first layer
							bTranspOn = true;
							usSavedLayer1 = uint16_t(ul);
						}
						else{
							FILL_COLOR(pDot, uint16_t(ul));
							break;
						}
					}
				}
				//otherwise nothing should be done but fall back to later layers, but maybe the right way is to end previously active layer1 in this situ
			}

			for ( uint16_t i = 0; i < 4; i++ ){
                if ( s_arrBgsPrio[i] == prio && ((byLayerMask >> i) & 0x01) != 0 ){
                    us = s_arrBgs[i]->Render(x);
                    if ( us != 0x8000 ){
						if ( bTranspOn ){
							if ( g_arrDevRegCache[0x51] & (1 << i) != 0 ){	//second layer found
								FILL_COLOR_BLENDING(pDot, usSavedLayer1, us);
							}
							else{	//just fill with the saved color
								FILL_COLOR(pDot, usSavedLayer1);
							}
							bTranspOn = false;
							prio = 4;
							break;
						}
						else{
							uint8_t by1 = 0xC0 | (1 << i);
							uint8_t by2 = 0x40 | (1 << i);
							if ( (g_arrDevRegCache[0x50] & by1) == by2 ){	//render it as the first layer
								bTranspOn = true;
								usSavedLayer1 = us;
							}
							else{
								FILL_COLOR(pDot, us);
								prio = 4;
								break;
							}
						}
					}
					//otherwise nothing should be done but fall back to later layers, but maybe the right way is to end previously active layer1 in this situ
                }
            }
        }
		if ( bTranspOn )
			FILL_COLOR(pDot, usSavedLayer1);
		pDot += 3;
    }
}

#include <typeinfo>

template <typename T>
void SetBgRender(int slot)
{
	static T bgs[4];
    if ( REG_BITS(uint16_t, 0, slot + 8, 1) == 0 ){
        s_arrBgsPrio[slot] = 0xFFFF;
        return;
    }
    s_arrBgsPrio[slot] = REG_BITS(uint16_t, 0x0008 + slot * 2, 0, 2);
    s_arrBgs[slot] = bgs + slot;
    s_arrBgs[slot]->ReloadCtl(0x0008 + slot * 2);
}

void DisplayRenderStart()
{
    if ( REG_BITS(uint16_t, 0, 7, 1) != 0 ){    //force blank
        s_bForceBlank = true;
        return;
    }
    s_bForceBlank = false;

	FILL_COLOR(s_BD, g_arrStorPalramBg[0]);
	s_eva = g_arrDevRegCache[0x52] & 0x1F;
	if ( s_eva > 16 ) s_eva = 16;
	s_evb = g_arrDevRegCache[0x53] & 0x1F;
	if ( s_evb > 16 ) s_evb = 16;
	s_bObjWindowOn = REG_BITS(uint16_t, 0, 15, 1);

	s_SprRdr.ReloadCtl();

    switch ( REG_BITS(uint16_t, 0, 0, 3) ){
    case 0:
        for ( uint16_t i = 0; i < 4; i++ ){
            SetBgRender<TiledBg>(i);
        }
        break;
    case 1:
        SetBgRender<TiledBg>(0);
        SetBgRender<TiledBg>(1);
        SetBgRender<TiledBgRS>(2);
        s_arrBgsPrio[3] = 0xFFFF;
        break;
    case 2:
        s_arrBgsPrio[0] = 0xFFFF;
        s_arrBgsPrio[1] = 0xFFFF;
        SetBgRender<TiledBgRS>(2);
        SetBgRender<TiledBgRS>(3);
        break;
    default:
        throw "display mode not supported";
    }
}

void DmaEvent(SystemEvent e);
extern AsgbaEvtHdlr g_funcOutEvtHdlr;

FASTCALL bool DoDispTicksUpdate()
{
    s_usTicksH += g_usTicksThisPiece;
    if ( s_usTicksH >= s_usTicksHNextEvt ){
        if ( s_usTicksHNextEvt == 1232 ){
            s_usTicksH -= 1232;
            s_usTicksHNextEvt = 960;
            s_usCurrLine = ( s_usCurrLine + 1 ) % 228;
            s_byHBlankOn = 0;

            if ( s_usCurrLine == uint16_t(g_arrDevRegCache[0x05]) ){
                s_byVCountOn = 4;
                if ( s_byVCountIntr != 0 )
                    SetIRQ(INTR_INDEX_VCNT);
            }
            else
                s_byVCountOn = 0;   //vcount is off every line unless reg 05 matches current line

            if ( s_usCurrLine < 160 ){ //hRend, should dma be notified?
                if ( s_usCurrLine == 0 ){   //vRend, should dma be notified?
                    s_byVBlankOn = 0;
                    DisplayRenderStart();
                }
                DisplayRenderLine();
            }
            else if ( s_usCurrLine == 160 ){//&& !s_bForceBlank ){ //VBlank
				if ( g_funcOutEvtHdlr(EVT_VBLK, NULL) == false ){
					return false;
				}
                //should force-blank supress the status? assume no
                s_byVBlankOn = 1;
                DmaEvent(EVT_VBLK);
                if ( s_byVBlankIntr != 0 )
                    SetIRQ(INTR_INDEX_VBLK);
            }
        }
        else if ( s_usTicksHNextEvt == 960 ){
            s_usTicksHNextEvt = 1232;
            s_byHBlankOn = 2;  //HBLANK toggled anyway
            if ( s_byHBlankIntr != 0 )  //should this be done in vblank?
                SetIRQ(INTR_INDEX_HBLK);
            if ( s_usCurrLine < 160 && !s_bForceBlank ){ //hBlank
                DmaEvent(EVT_HBLK);
                //if ( s_byHBlankIntr != 0 )
                //    SetIRQ(INTR_INDEX_HBLK);
            }
        }
    }
	return true;
}

static void DispGetVCnt(uint8_t size)
{
	g_arrDevRegCache[0x06] = uint8_t(s_usCurrLine); //should be exactly 0 on start, but the instrs before first disp start are not likely to access this
}

static void DispGetVStatusLow(uint8_t size)
{
	g_arrDevRegCache[0x04] = s_byVBlankOn | s_byHBlankOn | s_byVCountOn
        | s_byVBlankIntr | s_byHBlankIntr | s_byVCountIntr;
}

static void DispSetVStatusLow(uint8_t arrVal[], uint8_t size)
{
    s_byVBlankIntr = arrVal[0] & 0x08;
    s_byHBlankIntr = arrVal[0] & 0x10;
    s_byVCountIntr = arrVal[0] & 0x20;
    //don't really need to set the cache
}

void Init_Display(uint8_t *rgbbuf)
{
    //local vars
    s_usTicksH = 1232;
    s_usCurrLine = 227;
    s_usTicksHNextEvt = 1232;
    s_byVBlankOn = 0;
    s_byHBlankOn = 0;
    s_byVCountOn = 0;
    s_byVBlankIntr = 0;
    s_byHBlankIntr = 0;
    s_byVCountIntr = 0;

    //registers initialization
    *(uint16_t*)(g_arrDevRegCache + 0x0000) = 0x0080;   //DISPCNT
    *(uint16_t*)(g_arrDevRegCache + 0x0020) = 0x0100;   //dx
    *(uint16_t*)(g_arrDevRegCache + 0x0030) = 0x0100;   //dx
    *(uint16_t*)(g_arrDevRegCache + 0x0026) = 0x0100;   //dmy
    *(uint16_t*)(g_arrDevRegCache + 0x0036) = 0x0100;   //dmy

    //register handlers
   	RegisterDevRegHandler(0x0006, NULL, DispGetVCnt);
   	//vstatus, effective on every disp start? or instantly? assume instantly.
   	//only the lower part need to be handled, the higher half will be access via buffer directly
   	RegisterDevRegHandler(0x0004, DispSetVStatusLow, DispGetVStatusLow);

	g_arrRndrBuf = rgbbuf;
}
