/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef LSPTextDocument_H
#define LSPTextDocument_H

#include "MessageHandler.h"
#include <Url.h>

class LSPTextDocument : public MessageHandler {
public:
    LSPTextDocument(BPath filePath, BString fileType) {
		fFilenameURI = BUrl(filePath);
		fFilenameURI.SetAuthority("") ;
		fFileStatus = "";
		fFileType = fileType;
	}

    const BString	GetFilenameURI()  { return fFilenameURI.UrlString();}
	const BString	GetFileStatus()	  { return fFileStatus; }

			void	SetFileStatus(BString newStatus) { fFileStatus = newStatus; }
			void	SetFileType(BString newType) { fFileType = newType; }

	const BString FileType() { return fFileType; }

private:

	BUrl 	fFilenameURI;
	BString	fFileStatus;
	BString fFileType;
};


#endif // LSPTextDocument_H
