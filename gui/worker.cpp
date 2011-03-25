#include "worker.h"
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/file.h>
#include "main_frame.h"
#include "../asgba.h"

extern MainFrame *g_pMainFrame;

DEFINE_EVENT_TYPE(WORKER_NOTIFY_EVT)

Worker::Worker(): wxThread(wxTHREAD_JOINABLE)
{
}

Worker::~Worker()
{
}

FASTCALL bool EvtHdlr(SystemEvent evt, void *pParam)
{
	//do all sleep, pause, quiting... here
	if ( evt == EVT_VBLK ){
		//create image, bitmap, and send the bitmap to the main thread
		wxBitmap *pBmp = new wxBitmap(wxImage(320, 160, (unsigned char *)pParam, true));

		//notify the main thread
		wxCommandEvent event(WORKER_NOTIFY_EVT, 0);
		//event.SetEventObject(this);
		event.SetClientData(pBmp);
		::wxPostEvent(g_pMainFrame, event);
	}
	return true;
}

void* Worker::Entry()
{
	static uint8_t biosbuf[0x4000];
	wxFile bios;
	size_t sz = 0;
	if ( bios.Open(wxT("bios.bin")) == true ){
		sz = bios.Read(biosbuf, sizeof(biosbuf));
		bios.Close();
	}
	if ( sz == sizeof(biosbuf) ){
		AsgbaInit(EvtHdlr, biosbuf);
		AsgbaExec();
		//only quit when EvtHdlr return false
	}
	
	//notify the main thread
	wxCommandEvent event(WORKER_NOTIFY_EVT, 0);
    //event.SetEventObject(this);
    event.SetClientData(NULL);
	::wxPostEvent(g_pMainFrame, event);
    //GetEventHandler()->ProcessEvent( event );

	return NULL;
}
