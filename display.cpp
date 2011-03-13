#include "phymem.h"

//sys ticks related, actually not real cpu work, but anyway
static uint16_t s_usTicksH;
static uint16_t s_usTicksHNextEvt;
static uint16_t s_usCurrLine;

#include "display.hpp"

static DispLayer *s_arrObjLayers[4] = {NULL, NULL, NULL, NULL};    //indexed by priority
static DispLayer *s_arrBgs[4] = {NULL, NULL, NULL, NULL};  //indexed by bg number
static uint16_t s_arrBgsPrio[4];     //the priorities among bgs and objs is not clear
static bool s_bForceBlank;

void DisplayRenderLine()
{
}

template <typename T>
void SetBgRender(int slot)
{
    if ( REG_BITS(uint16_t, 0, slot + 8, 1) == 0 ){
        s_arrBgsPrio[slot] = 0xFFFF;
        return;
    }
    s_arrBgsPrio[slot] = REG_BITS(uint16_t, 0x0008 + slot * 2, 0, 2);
    if ( s_arrBgs[slot] != NULL )
        if ( typeid(*s_arrBgs[slot]) != typeid(T) ){
            delete s_arrBgs[slot];
            s_arrBgs[slot] = new T();
        }
    }
    else s_arrBgs[slot] = new T();
    s_arrBgs[slot].ReloadCtl(0x0008 + slot * 2);
}

void DisplayRenderStart()
{
    if ( REG_BITS(uint16_t, 0, 7, 1) != 0 ){    //force blank
        s_bForceBlank = true;
        return;
    }
    s_bForceBlank = false;

    switch ( REG_BITS(uint16_t, 0, 0, 3) ){
    case 0:
        for ( uint16_t i = 0; i < 4; i++ ){
            SetBgRender<TiledBg>(i);
        }
        break;
    case 1:
        for ( uint16_t i = 0; i < 2; i++ ){
            SetBgRender<TiledBg>(i);
        }
        SetBgRender<TiledBgRS>(2);
        s_arrBgsPrio[3] = 0xFFFF;
        break;
    case 1:
        for ( uint16_t i = 2; i < 4; i++ ){
            SetBgRender<TiledBgRS>(i);
        }
        s_arrBgsPrio[0] = 0xFFFF;
        s_arrBgsPrio[1] = 0xFFFF;
        break;

    }
}

void DoDispTicksUpdate()
{
    s_usTicksH += g_usTicksThisPiece;
    if ( s_usTicksH >= s_usTicksHNextEvt ){
        if ( s_usTicksHNextEvt == 1232 ){
            s_usTicksH -= 1232;
            s_usTicksHNextEvt = 960;
            s_usCurrLine++;
            if ( s_usCurrLine < 160 ){ //hRend, should dma be notified?
                DisplayRenderLine();
            }
            else if ( s_usCurrLine == 160 ){ //VBlank
                DmaEvent(EVT_VBLK);
                SetIRQ(INTR_INDEX_VBLK);
            }
            else if ( s_usCurrLine == 228 ){ //VRend, should dma be notified?
                s_usCurrLine = 0;
                DisplayRenderStart();
            }
        }
        else if ( s_usTicksHNextEvt == 960 ){
            s_usTicksHNextEvt = 1232;
            if ( s_usCurrLine < 160 ){ //hBlank
                DmaEvent(EVT_HBLK);
                SetIRQ(INTR_INDEX_HBLK);
            }
        }
    }
}

void Init_Display()
{
    //ticks
    s_usTicksH = 1232;
    s_usCurrLine = 0xFFFF;
    s_usTicksHNextEvt = 1232;

    //registers initialization
    *(uint16_t*)(g_arrDevRegCache + 0x0000) = 0x0080;   //DISPCNT
}
