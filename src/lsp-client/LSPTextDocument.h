/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#ifndef LSPTextDocument_H
#define LSPTextDocument_H

#include "MessageHandler.h"


class LSPTextDocument : public MessageHandler {
public:
    LSPTextDocument(BString fileURI) { fFilenameURI = fileURI; fFileStatus = "";}
    
    const BString	GetFilenameURI()  { return fFilenameURI;}
	const BString	GetFileStatus()	  { return fFileStatus; }
	
			void	SetFileStatus(BString newStatus) { fFileStatus = newStatus; }

private:

	BString fFilenameURI;
	BString	fFileStatus;
};


#endif // LSPTextDocument_H
