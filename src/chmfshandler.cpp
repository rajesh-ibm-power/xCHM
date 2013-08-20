/*

  Copyright (C) 2003 - 2013  Razvan Cojocaru <rzvncj@gmail.com>
 
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
	return (p == "xchm" 
		&& GetProtocol(GetLeftLocation(location)) == "file")
		|| !location.Left(6).CmpNoCase("MS-ITS");
}


wxFSFile* CHMFSHandler::OpenFile(wxFileSystem& WXUNUSED(fs), 
				 const wxString& location)
{
	wxString chmFile = GetRightLocation(location);
	wxString urlFile = GetLeftLocation(location);

	if(!location.Left(6).CmpNoCase("MS-ITS")) {
		urlFile = wxString("/") + location;
		chmFile = wxEmptyString;

	} else if (GetProtocol(chmFile) != "file")
		return NULL;

	// HTML code for space is %20
	urlFile.Replace("%20", " ", TRUE);
	urlFile.Replace("%5F", "_", TRUE);
	urlFile.Replace("%2E", ".", TRUE);
	urlFile.Replace("%2D", "-", TRUE);
	urlFile.Replace("%26", "&", TRUE);
            
	wxFileName filename = wxFileSystem::URLToFileName(chmFile);
	filename.Normalize();

	CHMInputStream *s = new CHMInputStream(chmFile.IsEmpty() ? 
		chmFile : filename.GetFullPath(), urlFile);

	if (s && s->IsOk()) {

		if(urlFile.IsSameAs("/"))
			urlFile = s->GetCache()->HomePage();

		// The dreaded links to files in other archives.
		// Talk about too much enthusiasm.
		if(!urlFile.Left(8).CmpNoCase("/MS-ITS:"))
			urlFile = urlFile.AfterLast(':');

		wxString tmp = wxString("file:") + urlFile +
			"#xchm:" + s->GetCache()->ArchiveName();

		return new wxFSFile(s, tmp,
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
