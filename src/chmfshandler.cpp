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


#include <chmfshandler.h>
#include <chminputstream.h>


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
	return (p == wxT("chm") 
		&& GetProtocol(GetLeftLocation(location)) == wxT("file"))
		|| !location.Left(6).CmpNoCase(wxT("MS-ITS"));
}


wxFSFile* CHMFSHandler::OpenFile(wxFileSystem& WXUNUSED(fs), 
				 const wxString& location)
{
	wxString right = GetRightLocation(location);
	wxString left = GetLeftLocation(location);
	CHMInputStream *s = NULL;

	if(!location.Left(6).CmpNoCase(wxT("MS-ITS"))) {
		right = wxString(wxT("/")) + location;
		left = wxEmptyString;

	} else if (GetProtocol(left) != wxT("file"))
		return NULL;
	
	// HTML code for space is %20
	right.Replace(wxT("%20"), wxT(" "), TRUE);
	right.Replace(wxT("%5F"), wxT("_"), TRUE);
	right.Replace(wxT("%2E"), wxT("."), TRUE);
	right.Replace(wxT("%2D"), wxT("-"), TRUE);
	right.Replace(wxT("%26"), wxT("&"), TRUE);
            
	wxFileName filename = wxFileSystem::URLToFileName(left);
	s = new CHMInputStream(left.IsEmpty() ? 
			       left : filename.GetFullPath(), right);

	if (s && s->IsOk()) {
		
		if(right.IsSameAs(wxT("/")))
			right = s->GetCache()->HomePage();

		// The dreaded links to files in other archives.
		// Talk about too much enthusiasm.
		if(!right.Left(8).CmpNoCase(wxT("/MS-ITS:")))
			right = right.AfterLast(wxT(':'));

		return new wxFSFile(s,
				    wxString(wxT("file:")) +
				    s->GetCache()->ArchiveName() + 
				    wxT("#chm:") + right,
				    GetMimeTypeFromExt(right.Lower()),
				    GetAnchor(location),
				    wxDateTime(wxFileModificationTime(left)));
	}
    
	delete s;
	return NULL;
}
