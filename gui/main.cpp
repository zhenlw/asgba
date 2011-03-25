/*********************************************************************
 * Name:      	main.cpp
 * Purpose:   	Implements simple wxWidgets application with GUI.
 * Author:    
 * Created:   
 * Copyright: 
 * License:   	wxWidgets license (www.wxwidgets.org)
 * 
 * Notes:		
 *********************************************************************/
 
#include <wx/wx.h>
#include "main_frame.h"

// application class
class wxMiniApp : public wxApp
{
    public:
		// function called at the application initialization
        virtual bool OnInit();

		// event handler for button click
        //void OnClick(wxCommandEvent& event) { GetTopWindow()->Close(); }
};

IMPLEMENT_APP(wxMiniApp);

MainFrame *g_pMainFrame;

bool wxMiniApp::OnInit()
{
	// create a new frame and set it as the top most application window
	g_pMainFrame = new MainFrame();
    SetTopWindow(g_pMainFrame);

	// create new button and assign it to the main frame
    //new wxButton( GetTopWindow(), wxID_EXIT, wxT("Click!") );

	// connect button click event with event handler
    //Connect(wxID_EXIT, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(wxMiniApp::OnClick) );

	// show main frame
    g_pMainFrame->Show();

	// enter the application's main loop
    return true;
}
