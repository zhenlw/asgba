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

static wxImage s_img(240, 160, false);

FASTCALL bool EvtHdlr(SystemEvent evt, void *pParam)
{
	//do all sleep, pause, quiting... here
	if ( g_bStopWorker )
		return false;	//don't think the mutex is necessary for reading

	if ( evt == EVT_VBLK ){
		//create image, bitmap, and send the bitmap to the main thread
		wxBitmap *pBmp = new wxBitmap(s_img);

		//notify the main thread
		wxCommandEvent event(WORKER_NOTIFY_EVT, 0);
		//event.SetEventObject(this);
		event.SetClientData(pBmp);
		::wxPostEvent(g_pMainFrame, event);
	}
	return true;
}

void Worker::Stop()
{
	wxCriticalSectionLocker locker(g_csWorker);

	g_bStopWorker = true;

	//Wait();	//must wait before the locker is off, or the ui may re start it before the thread end.
	//also the check of the flag cannot lock the critical section
}

void* Worker::Entry()
{
	static uint8_t biosbuf[0x4000];
	wxFile fl;
	size_t sz = 0;
	if ( fl.Open(wxT("bios.bin")) == true ){
		sz = fl.Read(biosbuf, sizeof(biosbuf));
		fl.Close();
	}
	uint8_t *pRom = NULL;
	if ( sz == sizeof(biosbuf) && fl.Open(wxT("rom.bin")) == true ){
		sz = fl.Length();
		if ( sz > 0 && sz <= (1UL << 23) ){
			pRom = new uint8_t[sz];
			fl.Read(pRom, sz);
		}
		fl.Close();
	}
	if ( pRom != NULL ){
		AsgbaInit(EvtHdlr, biosbuf, pRom, uint32_t(sz), s_img.GetData());
		AsgbaExec();
		//only quit when EvtHdlr return false
		delete pRom;
	}
	
	//notify the main thread
	wxCommandEvent event(WORKER_NOTIFY_EVT, 0);
    //event.SetEventObject(this);
    event.SetClientData(NULL);
	::wxPostEvent(g_pMainFrame, event);
    //GetEventHandler()->ProcessEvent( event );

	return NULL;
}

wxCriticalSection g_csWorker;
bool g_bStopWorker;
