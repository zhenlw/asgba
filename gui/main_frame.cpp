#include "main_frame.h"
#include <wx/wx.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(ID_Run,  MainFrame::OnRun)
    EVT_MENU(ID_Quit, MainFrame::OnQuit)
	EVT_COMMAND(0, WORKER_NOTIFY_EVT, MainFrame::OnNotifyFromWorker) 
END_EVENT_TABLE()

MainFrame::MainFrame()
	:wxFrame((wxFrame *)NULL, -1, wxT("asgba"), wxDefaultPosition, wxSize(360, 240))
{
	m_pBmp = NULL;

    wxMenu *menuFile = new wxMenu;

    menuFile->Append(ID_Run, wxT("&Run"));
    menuFile->AppendSeparator();
    menuFile->Append(ID_Quit, wxT("E&xit"));

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, wxT("&File"));

    SetMenuBar( menuBar );

    CreateStatusBar();
    SetStatusText(wxT("Welcome to wxWidgets!"));
}

MainFrame::~MainFrame()
{
	if ( NULL != m_pBmp ) delete m_pBmp;
}

void MainFrame::OnQuit(wxCommandEvent& e)
{
	Close();
}

void MainFrame::OnRun(wxCommandEvent& e)
{
	m_Worker.Create();
	m_Worker.Run();
}

void MainFrame::OnNotifyFromWorker(wxCommandEvent& e)
{
	if ( e.GetClientData() == NULL ){	//quiting event
		m_Worker.Wait();
		if ( NULL != m_pBmp ) delete m_pBmp;
		m_pBmp = NULL;
		return;
	}

	if ( NULL != m_pBmp ) delete m_pBmp;
	m_pBmp = (wxBitmap *)(e.GetClientData());
}
