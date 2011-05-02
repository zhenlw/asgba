#include "main_frame.h"
#include <wx/wx.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(ID_Run,  MainFrame::OnRun)
	EVT_MENU(ID_Stop,  MainFrame::OnStop)
    EVT_MENU(ID_Quit, MainFrame::OnQuit)
	EVT_CLOSE(MainFrame::OnClose)
	EVT_COMMAND(0, WORKER_NOTIFY_EVT, MainFrame::OnNotifyFromWorker)
	EVT_PAINT(MainFrame::OnPaint)
END_EVENT_TABLE()

MainFrame::MainFrame()
	:wxFrame((wxFrame *)NULL, -1, wxT("asgba"), wxDefaultPosition, wxSize(300, 280))
{
	m_pBmp = NULL;

    wxMenu *menuFile = new wxMenu;

    menuFile->Append(ID_Run, wxT("&Run"));
    menuFile->Append(ID_Stop, wxT("&Stop"));
    menuFile->AppendSeparator();
    menuFile->Append(ID_Quit, wxT("E&xit"));

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, wxT("&File"));

    SetMenuBar( menuBar );

    CreateStatusBar();
    SetStatusText(wxT("Welcome to wxWidgets!"));
	m_pWorker = NULL;
}

MainFrame::~MainFrame()
{
	if ( NULL != m_pBmp ) delete m_pBmp;
}

void MainFrame::OnClose(wxCloseEvent &e)
{
	if ( m_pWorker != NULL ){
		m_pWorker->Stop();
		m_pWorker->Wait();
		delete m_pWorker;
		m_pWorker = NULL;
	}
	Destroy();
}

void MainFrame::OnQuit(wxCommandEvent& e)
{
	Close();
}

void MainFrame::OnRun(wxCommandEvent& e)
{
	if ( m_pWorker != NULL ) return;

	m_pWorker = new Worker();
	m_pWorker->Create();
	//wxCriticalSectionLocker locker(g_csWorker);
	g_bStopWorker = false;

	m_pWorker->Run();
}

void MainFrame::OnStop(wxCommandEvent& e)
{
	if ( m_pWorker != NULL ){
		m_pWorker->Stop();
		m_pWorker->Wait();
		delete m_pWorker;
		m_pWorker = NULL;
	}
}

void MainFrame::OnNotifyFromWorker(wxCommandEvent& e)
{
	if ( e.GetClientData() == NULL ){	//quiting event
		//m_Worker.Wait();
		if ( NULL != m_pBmp ) delete m_pBmp;
		m_pBmp = NULL;
		return;
	}

	if ( NULL != m_pBmp ) delete m_pBmp;
	m_pBmp = (wxBitmap *)(e.GetClientData());
	this->Refresh();
}

void MainFrame::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	if ( m_pBmp != NULL )
		dc.DrawBitmap(*m_pBmp, 0, 0);
}
