/*

  Copyright (C) 2003  Razvan Cojocaru <razvanco@gmx.net>
 
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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/


#include <chmhtmlwindow.h>
#include <contenttaghandler.h>
#include <chminputstream.h>



CHMHtmlWindow::CHMHtmlWindow(wxWindow *parent, wxTreeCtrl *tc)
	: wxHtmlWindow(parent, -1, wxDefaultPosition, wxSize(200,200)),
	  _tcl(tc), _syncTree(true), _found(false), _menu(NULL)
#ifdef _ENABLE_COPY_AND_FIND
	, _fdlg(NULL)
#endif
{
	_menu = new wxMenu;
	_menu->Append(ID_PopupForward, wxT("Forward"));
	_menu->Append(ID_PopupBack, wxT("Back"));

#ifdef _ENABLE_COPY_AND_FIND
	_menu->AppendSeparator();
	_menu->Append(ID_CopySel, wxT("Copy selection"));
	_menu->AppendSeparator();
	_menu->Append(ID_PopupFind, wxT("Find in page .."));
	
	wxWindow* p = parent;
	while(p->GetParent())
		p = p->GetParent();

	_fdlg = new CHMFindDialog(p, this);
#endif
}


CHMHtmlWindow::~CHMHtmlWindow()
{
	delete _menu;
#ifdef _ENABLE_COPY_AND_FIND
	delete _fdlg;
#endif
}


bool CHMHtmlWindow::LoadPage(const wxString& location)
{
	wxString tmp = location;

	// handle relative paths
	if(tmp.StartsWith(wxT(".."))) {
		CHMFile *chmf = CHMInputStream::GetCache();

		if(chmf)
			tmp.Replace(wxT(".."), wxString(wxT("file:")) + 
				    chmf->ArchiveName() + wxT("#chm:") +
				    GetPrefix().BeforeLast(wxT('/')));
	
	} else if(tmp.StartsWith(wxT("javascript:fullSize"))) {
		tmp = tmp.AfterFirst(wxT('\'')).BeforeLast(wxT('\''));
	}

	if(_syncTree && 
	   // We should be looking for a valid page, not / (home).
	   !location.AfterLast(wxT('/')).IsEmpty() && 
	   _tcl->GetCount() > 1) {

		// Sync will call SelectItem() on the tree item
		// if it finds one, and that in turn will call
		// LoadPage() with _syncTree set to false.
		Sync(_tcl->GetRootItem(), tmp);

		if(_found)
			_found = false;
	}

	return wxHtmlWindow::LoadPage(tmp);
}


void CHMHtmlWindow::Sync(wxTreeItemId root, const wxString& page)
{
	if(_found)
		return;

	URLTreeItem *data = reinterpret_cast<URLTreeItem *>(
		_tcl->GetItemData(root));

	wxString url, tmp;

	if(page.StartsWith(wxT("/")) || page.StartsWith(wxT("file:")))
		tmp = page.AfterLast(wxT(':')).AfterFirst(
			wxT('/')).BeforeFirst(wxT('#')).Lower();
	else
		tmp = page.BeforeFirst(wxT('#')).Lower();

	if(data)
		url = (data->_url).BeforeFirst(wxT('#')).Lower();

	if(data && (url == tmp || url == GetPrefix() + wxT("/") + tmp)) {
		// Order counts!
		_found = true;
		_tcl->SelectItem(root);
		return;
	}

	long cookie;
	wxTreeItemId child = _tcl->GetFirstChild(root, cookie);

	for(size_t i = 0; i < _tcl->GetChildrenCount(root, FALSE); ++i) {
		Sync(child, page);
		child = _tcl->GetNextChild(root, cookie);
	}
}


inline 
wxString CHMHtmlWindow::GetPrefix() const
{
	return GetOpenedPage().AfterLast(wxT(':')).AfterFirst(
		wxT('/')).BeforeLast(wxT('/')).Lower();
}


#ifdef _ENABLE_COPY_AND_FIND


wxHtmlCell* CHMHtmlWindow::FindFirst(wxHtmlCell *parent, const wxString& word,
				     bool wholeWords, bool caseSensitive)
{
	wxString tmp = word;
	
	if(!parent)
		return NULL;
 
	if(!caseSensitive)
		tmp.MakeLower();

	// If this cell is not a container, the for body will never happen
	// (GetFirstChild() will return NULL).
	for(wxHtmlCell *cell = parent->GetFirstChild(); cell; 
	    cell = cell->GetNext()) {
		
		wxHtmlCell *result;
		if((result = FindFirst(cell, word, wholeWords, caseSensitive)))
			return result;
	}

	wxHtmlSelection ws;
	ws.Set(parent, parent);
	wxString text = parent->ConvertToText(&ws);

	if(text.IsEmpty())
		return NULL;

	if(!caseSensitive)
		text.MakeLower();

	text.Trim(TRUE);
	text.Trim(FALSE);

	bool found = false;

	if(wholeWords && text == tmp) {
		found = true;
	} else if(!wholeWords && text.Find(tmp.c_str()) != -1) {
		found = true;
	}

	if(found) {
		int w, h;
		GetSize(&w, &h);

		// What is all this wxWindows protected member crap?
		delete m_selection;
		m_selection = new wxHtmlSelection();

		// The wxPoint(w,h) is a hack because Set(parent, parent)
		// doesn't select the whole word on screen.
		m_selection->Set(wxDefaultPosition, parent, wxPoint(w,h), 
				 parent);		
		int y;
		wxHtmlCell *cell = parent;

		for (y = 0; cell != NULL; cell = cell->GetParent()) 
			y += cell->GetPosY();
		Scroll(-1, y / wxHTML_SCROLL_STEP);
		Refresh();
		
		return parent;
	}

	return NULL;
}


wxHtmlCell* CHMHtmlWindow::FindNext(wxHtmlCell *start, const wxString& word, 
				    bool wholeWords, bool caseSensitive)
{
	wxHtmlCell *cell;

	if(!start)
		return NULL;

	for(cell = start; cell; cell = cell->GetNext()) {
		wxHtmlCell *result;
		if((result = FindFirst(cell, word, wholeWords, caseSensitive)))
			return result;
	}

	cell = start->GetParent();
	
	while(cell && !cell->GetNext())
		cell = cell->GetParent();

	if(!cell)
		return NULL;

	return FindNext(cell->GetNext(), word, wholeWords, caseSensitive);
}


void CHMHtmlWindow::ClearSelection()
{
	delete m_selection;
	m_selection = NULL;
	Refresh();
}


void CHMHtmlWindow::OnCopy(wxCommandEvent& WXUNUSED(event))
{
	CopySelection();
}


void CHMHtmlWindow::OnFind(wxCommandEvent& WXUNUSED(event))
{
	
	_fdlg->ShowModal();
}

#endif // _ENABLE_COPY_AND_FIND


void CHMHtmlWindow::OnForward(wxCommandEvent& WXUNUSED(event))
{
	HistoryForward();
}


void CHMHtmlWindow::OnBack(wxCommandEvent& WXUNUSED(event))
{
	HistoryBack();
}


void CHMHtmlWindow::OnRightClick(wxMouseEvent& event)
{
#ifdef _ENABLE_COPY_AND_FIND
	if(IsSelectionEnabled())
		_menu->Enable(ID_CopySel, m_selection != NULL);
#endif

	_menu->Enable(ID_PopupForward, HistoryCanForward());
	_menu->Enable(ID_PopupBack, HistoryCanBack());

	PopupMenu(_menu, event.GetPosition());
}


BEGIN_EVENT_TABLE(CHMHtmlWindow, wxHtmlWindow)
#ifdef _ENABLE_COPY_AND_FIND
	EVT_MENU(ID_CopySel, CHMHtmlWindow::OnCopy)
	EVT_MENU(ID_PopupFind, CHMHtmlWindow::OnFind)
#endif
	EVT_MENU(ID_PopupForward, CHMHtmlWindow::OnForward)
	EVT_MENU(ID_PopupBack, CHMHtmlWindow::OnBack)
	EVT_RIGHT_DOWN(CHMHtmlWindow::OnRightClick)
END_EVENT_TABLE()



