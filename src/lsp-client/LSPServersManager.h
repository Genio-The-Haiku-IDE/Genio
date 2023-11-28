/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LSPServersManager_H
#define LSPServersManager_H


#include <SupportDefs.h>

class LSPProjectWrapper;
class LSPTextDocument;
class BMessenger;
class BPath;
class BString;

class LSPServerConfigInterface {
public:
	virtual const bool   IsFileTypeSupported (const BString& fileType) const = 0;
	virtual const char** CreateServerStartupArgs(int32& argc) const = 0;
};

class LSPServersManager {
public:
		static LSPProjectWrapper*	CreateLSPProject(const BPath& path, const BMessenger& msgr, const BString& fileType);

};


#endif // LSPServersManager_H
