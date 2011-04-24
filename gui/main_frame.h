#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <wx/frame.h> // Base class: wxFrame
#include "worker.h"

enum {
	ID_Run = 1, ID_Stop, ID_Quit
};

class MainFrame : public wxFrame {

public:
	MainFrame();
	~MainFrame();
	
	void OnQuit(wxCommandEvent &e);
	void OnRun(wxCommandEvent &e);
	void OnStop(wxCommandEvent &e);
	void OnNotifyFromWorker(wxCommandEvent &e);
	void OnClose(wxCloseEvent &e);
	void OnPaint(wxPaintEvent& event);

private:
    DECLARE_EVENT_TABLE()

	wxBitmap *m_pBmp;
	Worker m_Worker;
};

#endif // MAINFRAME_H
