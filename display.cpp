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

#define DISPLAY_IMPL
#include "display.hpp"

//static DispLayer *s_arrObjLayers[4] = {NULL, NULL, NULL, NULL};    //indexed by priority
//static DispLayer *s_arrBgs[4] = {NULL, NULL, NULL, NULL};  //indexed by bg slot
//static uint16_t s_arrBgsPrio[4];     //the priorities among bgs and objs is not clear
static bool s_bForceBlank;

static uint8_t s_BD[3];
static uint16_t s_eva, s_evb;
static uint16_t s_bObjWindowOn;


void DisplayRenderStart()
{
	DispContext::ReloadCtl();
	return;
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
                //DisplayRenderLine();
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

void Init_Display()
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

	DispContext::Init();
}
