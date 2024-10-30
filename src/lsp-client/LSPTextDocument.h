/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

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

    const BString	GetFilenameURI() const { return fFilenameURI.UrlString();}
	const BString	GetFileStatus()	const { return fFileStatus; }

			void	SetFileStatus(BString newStatus) { fFileStatus = newStatus; }
			void	SetFileType(BString newType) { fFileType = newType; }

	const BString& FileType() const { return fFileType; }

private:
	BUrl 	fFilenameURI;
	BString	fFileStatus;
	BString fFileType;
};
