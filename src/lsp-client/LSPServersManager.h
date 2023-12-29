/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LSPServersManager_H
#define LSPServersManager_H


#include <SupportDefs.h>
#include <string>
#include <vector>

class LSPProjectWrapper;
class LSPTextDocument;
class BMessenger;
class BPath;
class BString;

class LSPServerConfigInterface {
public:
	virtual const bool   IsFileTypeSupported (const BString& fileType) const = 0;
	const char* const* Argv() const { return fArgv.data(); }
				int32  Argc() const { return fArgv.size(); }

protected:
	std::vector<const char*>	fArgv;
};

class LSPServersManager {
public:
		static LSPProjectWrapper*	CreateLSPProject(const BPath& path, const BMessenger& msgr, const BString& fileType);

};


#endif // LSPServersManager_H
