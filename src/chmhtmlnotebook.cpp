/*

  Copyright (C) 2003 - 2013  Razvan Cojocaru <rzvncj@gmail.com>
  Tabbed browsing support developed by Cedric Boudinet <bouced@gmx.fr>
  (this file originally written by Cedric Boudinet)
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
  MA 02110-1301, USA.

*/


#include <chmframe.h>
#include <chmhtmlnotebook.h>
#include <chmfshandler.h>
#include <wx/webviewfshandler.h>
#include <wx/webview.h>



class MyHandler : public wxWebViewHandler {


public:
	MyHandler(const wxString& scheme)
	: wxWebViewHandler(scheme)
	{
		_fileSystem = new wxFileSystem;
	}

	~MyHandler() { delete _fileSystem; }

	wxFSFile* GetFile(const wxString& uri)
	{
		size_t pos = uri.find(wxT("//"));

		if(pos == wxString::npos)
			return NULL;

		wxString file = uri.substr(pos + 2);

		if(!file.Contains(wxT("xchm")))
			file += wxT("#xchm:");

		return _fileSystem->OpenFile(file);
	}

private:
	wxFileSystem* _fileSystem;
};



CHMHtmlNotebook::CHMHtmlNotebook(wxWindow *parent, wxTreeCtrl *tc,
				 CHMFrame* frame)
	: wxAuiNotebook(parent, -1, wxDefaultPosition, wxDefaultSize,
			wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_TAB_FIXED_WIDTH),
	  _tcl(tc), _frame(frame)
{
	const int NO_ACCELERATOR_ENTRIES = 3;

	wxAcceleratorEntry entries[NO_ACCELERATOR_ENTRIES];
	entries[0].Set(wxACCEL_CTRL,   WXK_PAGEUP,     ID_PriorPage);
	entries[1].Set(wxACCEL_CTRL,   WXK_PAGEDOWN,   ID_NextPage);
	entries[2].Set(wxACCEL_NORMAL, WXK_ESCAPE,     ID_OutOfFullScreen);

	wxAcceleratorTable accel(NO_ACCELERATOR_ENTRIES, entries);
	this->SetAcceleratorTable(accel);
	SetTabCtrlHeight(0);

	AddHtmlView(wxEmptyString, wxT("memory:about.html"));
	//AddHtmlView(wxEmptyString, wxT("http://www.google.com"));
}


wxWebView* CHMHtmlNotebook::CreateView()
{
	wxWebView* htmlWin = wxWebView::New(this, wxID_ANY);

	htmlWin->RegisterHandler(wxSharedPtr<wxWebViewHandler>(
					 new wxWebViewFSHandler("memory")));

	htmlWin->RegisterHandler(wxSharedPtr<wxWebViewHandler>(
					 new MyHandler("chmfs")));
	
	//htmlWin->SetRelatedFrame(_frame, wxT("xCHM v. ") wxT(VERSION));
	//htmlWin->SetRelatedStatusBar(0);

	Connect(htmlWin->GetId(), wxEVT_WEBVIEW_TITLE_CHANGED,
		wxWebViewEventHandler(CHMHtmlNotebook::OnTitleChanged),
		NULL, this);

	Connect(htmlWin->GetId(), wxEVT_WEBVIEW_ERROR,
		wxWebViewEventHandler(CHMHtmlNotebook::OnLoadError),
		NULL, this);

	Connect(htmlWin->GetId(), wxEVT_WEBVIEW_NAVIGATING,
		wxWebViewEventHandler(CHMHtmlNotebook::OnNavigating),
		NULL, this);

	Connect(htmlWin->GetId(), wxEVT_WEBVIEW_NAVIGATED,
		wxWebViewEventHandler(CHMHtmlNotebook::OnNavigated),
		NULL, this);

	Connect(htmlWin->GetId(), wxEVT_WEBVIEW_LOADED,
		wxWebViewEventHandler(CHMHtmlNotebook::OnLoaded),
		NULL, this);

	Connect(wxID_ANY, wxEVT_IDLE,
		wxIdleEventHandler(CHMHtmlNotebook::OnIdle),
		NULL, this);

	// It would have been better to give the URL directly to
	// wxWebView::New(), but file: and memory: handlers need to be
	// registered before anything useful is loaded. So, remove the
	// about:blank page.
	htmlWin->ClearHistory();

	AddPage(htmlWin, _("(Empty page)"));
	SetSelection(GetPageCount() - 1);	

	return htmlWin;
}


void CHMHtmlNotebook::AddHtmlView(const wxString& path,
				  const wxString& link)
{
	wxWebView *htmlWin = CreateView();
	
	if(htmlWin)
		htmlWin->LoadURL(link);
}


bool CHMHtmlNotebook::LoadPageInCurrentView(const wxString& location)
{
	wxWebView* htmlWin = GetCurrentPage();

	if(htmlWin) {
		// htmlWin->Stop();
		htmlWin->LoadURL(location);
	}

	return true;
}


// TODO: this is a misleading named function with side effects. It's a no-no.
wxWebView* CHMHtmlNotebook::GetCurrentPage()
{
	int selection = GetSelection();

	if(selection == wxNOT_FOUND)
		return CreateView();

	return dynamic_cast<wxWebView *>(
		wxAuiNotebook::GetPage(selection));
}


void CHMHtmlNotebook::OnGoToNextPage(wxCommandEvent&)
{
	int selection = GetSelection();

	if(selection >= static_cast<int>(GetPageCount() - 1))
		return;

	SetSelection(selection + 1);
}


void CHMHtmlNotebook::OnGoToPriorPage(wxCommandEvent&)
{
	int selection = GetSelection();

	if(selection <= 0)
		return;

	SetSelection(selection - 1);
}


void CHMHtmlNotebook::OnCloseTab(wxCommandEvent& WXUNUSED(event))
{
	DeletePage(GetSelection());

	if(GetPageCount() <= 1)
		SetTabCtrlHeight(0);
}


void CHMHtmlNotebook::OnNewTab(wxCommandEvent& WXUNUSED(event))
{
	AddHtmlView(wxEmptyString, wxEmptyString);
}


void CHMHtmlNotebook::OnOutOfFullScreen(wxCommandEvent& WXUNUSED(event))
{
	_frame->ToggleFullScreen(true);
}


void CHMHtmlNotebook::CloseAllPagesExceptFirst()
{
	SetSelection(0);

	while(GetPageCount() > 1)
		DeletePage(1);

	SetTabCtrlHeight(0);
}


bool CHMHtmlNotebook::AddPage(wxWindow* page, const wxString& title)
{
	if(!page)
		return false;

	bool st = wxAuiNotebook::AddPage(page, title);

	if(GetPageCount() == 2)
		SetTabCtrlHeight(-1);

	return st;
}


void CHMHtmlNotebook::OnPageChanged(wxAuiNotebookEvent&)
{
	if(GetPageCount() == 1)
		SetTabCtrlHeight(0);
}


void CHMHtmlNotebook::OnTitleChanged(wxWebViewEvent& evt)
{
	SetPageText(GetSelection(), evt.GetString());
}


void CHMHtmlNotebook::OnNavigating(wxWebViewEvent& evt)
{
	std::cout << "Navigating: " << evt.GetURL().mb_str()
		  << std::endl;
}


void CHMHtmlNotebook::OnNavigated(wxWebViewEvent& evt)
{
	std::cout << "Navigated: " << evt.GetURL().mb_str()
		  << std::endl;
}


void CHMHtmlNotebook::OnLoaded(wxWebViewEvent& evt)
{
	std::cout << "Document loaded: " << evt.GetURL().mb_str()
		  << std::endl;
}


void CHMHtmlNotebook::OnLoadError(wxWebViewEvent& evt)
{
#define WX_ERROR_CASE(type)			\
	case type:				\
		category = #type;		\
		break;
	
	wxString category;
	switch (evt.GetInt()) {
		WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_CONNECTION);
		WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_CERTIFICATE);
		WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_AUTH);
		WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_SECURITY);
		WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_NOT_FOUND);
		WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_REQUEST);
		WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_USER_CANCELLED);
		WX_ERROR_CASE(wxWEBVIEW_NAV_ERR_OTHER);
	}


	std::cout << "Error; url='" << evt.GetURL().mb_str()
		  << "', error='" << category.mb_str() << " ("
		  << evt.GetString().mb_str() << ")'" << std::endl;
}


void CHMHtmlNotebook::OnIdle(wxIdleEvent& WXUNUSED(evt))
{
	if(GetCurrentPage()->IsBusy()) {
		wxSetCursor(wxCURSOR_ARROWWAIT);
	} else {
		wxSetCursor(wxNullCursor);
	}
}


BEGIN_EVENT_TABLE(CHMHtmlNotebook, wxAuiNotebook)
	EVT_MENU(ID_PriorPage, CHMHtmlNotebook::OnGoToPriorPage)
	EVT_MENU(ID_NextPage, CHMHtmlNotebook::OnGoToNextPage)
	EVT_MENU(ID_OutOfFullScreen, CHMHtmlNotebook::OnOutOfFullScreen)
	EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, CHMHtmlNotebook::OnPageChanged)
END_EVENT_TABLE()


/*
  Local Variables:
  mode: c++
  c-basic-offset: 8
  tab-width: 8
  c-indent-comments-syntactically-p: t
  c-tab-always-indent: t
  indent-tabs-mode: t
  End:
*/

// vim:shiftwidth=8:autoindent:tabstop=8:noexpandtab:softtabstop=8


