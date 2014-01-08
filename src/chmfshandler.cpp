/*

  Copyright (C) 2003 - 2014  Razvan Cojocaru <rzvncj@gmail.com>
 
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


#include <chmfshandler.h>
#include <chminputstream.h>
#include <iostream>


// only needs to be here because I killed the constructors
// by providing a private copy constructor
CHMFSHandler::CHMFSHandler()
{
}


CHMFSHandler::~CHMFSHandler()
{
	CHMInputStream::Cleanup();
}


bool CHMFSHandler::CanOpen(const wxString& location)
{
	wxString p = GetProtocol(location);
	return (p == wxT("xchm")
		&& GetProtocol(GetLeftLocation(location)) == wxT("file"))
		|| !location.Left(6).CmpNoCase(wxT("MS-ITS"));
}


wxFSFile* CHMFSHandler::OpenFile(wxFileSystem& WXUNUSED(fs), 
				 const wxString& location)
{
	wxString chmFile = GetLeftLocation(location);
	wxString urlFile = GetRightLocation(location);

	if(!location.Left(6).CmpNoCase(wxT("MS-ITS"))) {
		urlFile = wxString(wxT("/")) + location;
		chmFile = wxEmptyString;

	} else if (GetProtocol(chmFile) != wxT("file"))
		return NULL;

	// HTML code for space is %20
	urlFile.Replace(wxT("%20"), wxT(" "), TRUE);
	urlFile.Replace(wxT("%5F"), wxT("_"), TRUE);
	urlFile.Replace(wxT("%2E"), wxT("."), TRUE);
	urlFile.Replace(wxT("%2D"), wxT("-"), TRUE);
	urlFile.Replace(wxT("%26"), wxT("&"), TRUE);
            
	wxFileName filename = wxFileSystem::URLToFileName(chmFile);
	filename.Normalize();

	CHMInputStream *s = new CHMInputStream(chmFile.IsEmpty() ? 
		chmFile : filename.GetFullPath(), urlFile);

	if (s && s->IsOk()) {

		if(urlFile.IsSameAs(wxT("/")))
			urlFile = s->GetCache()->HomePage();

		// The dreaded links to files in other archives.
		// Talk about too much enthusiasm.
		if(!urlFile.Left(8).CmpNoCase(wxT("/MS-ITS:")))
			urlFile = urlFile.AfterLast(wxT(':'));

		wxString name = wxString(wxT("file:")) + urlFile +
			wxT("#xchm:") + s->GetCache()->ArchiveName();

		return new wxFSFile(s, name,
				    GetMimeTypeFromExt(urlFile.Lower()),
				    GetAnchor(location),
				    wxDateTime((time_t)-1));
	}
    
	delete s;
	return NULL;
}


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
