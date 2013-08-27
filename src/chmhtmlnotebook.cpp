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
#include <hhcparser.h>
#include <wx/webviewfshandler.h>
#include <wx/webviewarchivehandler.h>
#include <wx/webview.h>



CHMHtmlNotebook::CHMHtmlNotebook(wxWindow *parent, wxTreeCtrl *tc,
				 CHMFrame* frame)
	: wxAuiNotebook(parent, -1, wxDefaultPosition, wxDefaultSize,
			wxAUI_NB_DEFAULT_STYLE | wxAUI_NB_TAB_FIXED_WIDTH),
	  _tcl(tc), _frame(frame), _syncTree(true), _isCaller(false)
{
	const int NO_ACCELERATOR_ENTRIES = 3;

	wxAcceleratorEntry entries[NO_ACCELERATOR_ENTRIES];
	entries[0].Set(wxACCEL_CTRL,   WXK_PAGEUP,     ID_PriorPage);
	entries[1].Set(wxACCEL_CTRL,   WXK_PAGEDOWN,   ID_NextPage);
	entries[2].Set(wxACCEL_NORMAL, WXK_ESCAPE,     ID_OutOfFullScreen);

	wxAcceleratorTable accel(NO_ACCELERATOR_ENTRIES, entries);
	this->SetAcceleratorTable(accel);
	SetTabCtrlHeight(0);

	AddHtmlView(wxT("memory:about.html"));
}


wxWebView* CHMHtmlNotebook::CreateView()
{
	wxWebView* htmlWin = wxWebView::New(this, wxID_ANY);

	htmlWin->RegisterHandler(wxSharedPtr<wxWebViewHandler>(
					 new wxWebViewFSHandler("memory")));

	htmlWin->RegisterHandler(wxSharedPtr<wxWebViewHandler>(
					 new wxWebViewArchiveHandler("chmfs")));
	
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

	Connect(htmlWin->GetId(), wxEVT_WEBVIEW_NEWWINDOW,
		wxWebViewEventHandler(CHMHtmlNotebook::OnNewWindow),
		NULL, this);

	Connect(wxID_ANY, wxEVT_IDLE,
		wxIdleEventHandler(CHMHtmlNotebook::OnIdle),
		NULL, this);

	AddPage(htmlWin, _("(Empty page)"));

	// It would have been better to give the URL directly to
	// wxWebView::New(), but file: and memory: handlers need to be
	// registered before anything useful is loaded. So, remove the
	// about:blank page.
	// htmlWin->ClearHistory();
	// htmlWin->EnableHistory();

	SetSelection(GetPageCount() - 1);	

	return htmlWin;
}


void CHMHtmlNotebook::AddHtmlView(const wxString& link)
{
	wxWebView *htmlWin = CreateView();
	
	if(htmlWin)
		htmlWin->LoadURL(link);
}


bool CHMHtmlNotebook::LoadPageInCurrentView(const wxString& location)
{
	wxWebView* htmlWin = GetCurrentPage();

	if(htmlWin) {
		htmlWin->Stop();
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
	AddHtmlView(wxEmptyString);
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
	_frame->UpdateHistoryTools();

	if(GetPageCount() == 1)
		SetTabCtrlHeight(0);
}


void CHMHtmlNotebook::OnTitleChanged(wxWebViewEvent& evt)
{
	SetPageText(GetSelection(), evt.GetString());
}


void CHMHtmlNotebook::OnNavigating(wxWebViewEvent& evt)
{
	if(_syncTree) {

		wxString page = evt.GetURL();

		size_t pos = page.find(wxT("=xchm"));

		if(pos != wxString::npos)
			page = page.substr(pos + 5);

		page = page.BeforeFirst(wxT('#'));

		_isCaller = true;
		Sync(_tcl->GetRootItem(), page);
		_isCaller = false;

	} else {
		_syncTree = true;
	}
}


void CHMHtmlNotebook::OnNavigated(wxWebViewEvent& /* evt */)
{
}


void CHMHtmlNotebook::Sync(wxTreeItemId root, const wxString& page)
{
        URLTreeItem *data = reinterpret_cast<URLTreeItem *>(
                _tcl->GetItemData(root));

        wxString url;

        if(data)
                url = (data->_url).BeforeFirst(wxT('#'));

        if(data && !url.CmpNoCase(page)) {
                _tcl->SelectItem(root);
                return;
        }

        wxTreeItemIdValue cookie;
        wxTreeItemId child = _tcl->GetFirstChild(root, cookie);

        for(size_t i = 0; i < _tcl->GetChildrenCount(root, FALSE); ++i) {
                Sync(child, page);
                child = _tcl->GetNextChild(root, cookie);
        }
}


void CHMHtmlNotebook::OnLoaded(wxWebViewEvent& evt)
{
	_frame->UpdateHistoryTools();

	wxStatusBar *sb = _frame->GetStatusBar();

	if(!sb)
		return;

	sb->SetStatusText(wxString(_("Loaded document: ")) + evt.GetURL());
}


void CHMHtmlNotebook::OnNewWindow(wxWebViewEvent& evt)
{
	std::cout << "New window: " << evt.GetURL().mb_str()
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

	wxStatusBar *sb = _frame->GetStatusBar();

	if(!sb)
		return;

	wxString status = wxString(_("Error; url='")) + evt.GetURL()
		+ _("', error='") + category + wxT(" (")
		+ evt.GetString() + wxT(")'");

	sb->SetStatusText(status);
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


