/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#ifndef LSPTextDocument_H
#define LSPTextDocument_H

#include "MessageHandler.h"
#include <Url.h>

class LSPTextDocument : public MessageHandler {
public:
    LSPTextDocument(BPath filePath) { fFilenameURI = BUrl(filePath); fFilenameURI.SetAuthority("") ; fFileStatus = "";}
    
    const BString	GetFilenameURI()  { return fFilenameURI.UrlString();}
	const BString	GetFileStatus()	  { return fFileStatus; }
	
			void	SetFileStatus(BString newStatus) { fFileStatus = newStatus; }

private:

	BUrl 	fFilenameURI;
	BString	fFileStatus;
};


#endif // LSPTextDocument_H
