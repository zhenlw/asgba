#include "main_frame.h"
#include <wx/wx.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(ID_Run,  MainFrame::OnRun)
	EVT_MENU(ID_Stop,  MainFrame::OnStop)
    EVT_MENU(ID_Quit, MainFrame::OnQuit)
	EVT_CLOSE(MainFrame::OnClose)
	EVT_COMMAND(0, WORKER_NOTIFY_EVT, MainFrame::OnNotifyFromWorker)
	//EVT_PAINT(MainFrame::OnPaint)
END_EVENT_TABLE()

MainFrame::MainFrame()
	:wxFrame((wxFrame *)NULL, -1, wxT("asgba"), wxDefaultPosition)
{
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

	this->SetClientSize(480, 320);
}

MainFrame::~MainFrame()
{
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

	this->SetClientSize(480, 320);
	m_pCanvas = new wxGLCanvas(this, wxID_ANY, 0, wxDefaultPosition, wxSize(480, 320));
	m_pContext = new wxGLContext(m_pCanvas);
	m_pContext->SetCurrent(*m_pCanvas);
	glOrtho(0.0, 240.0, 0.0, 160.0, -1.0, 1.0);

	m_pWorker->Run();
}

void MainFrame::OnStop(wxCommandEvent& e)
{
	if ( m_pWorker != NULL ){
		m_pWorker->Stop();
		m_pWorker->Wait();
		delete m_pWorker;
		delete m_pContext;
		delete m_pCanvas;
		m_pWorker = NULL;
	}
}

void DispCtxToGl();

void MainFrame::OnNotifyFromWorker(wxCommandEvent& e)
{
	if ( e.GetClientData() == NULL ){	//quiting event
		//m_Worker.Wait();
		return;
	}

	DispCtxToGl();
	glFlush();
	m_pCanvas->SwapBuffers();


	//this->Refresh();
}

