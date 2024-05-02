/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LSPServersManager_H
#define LSPServersManager_H


#include <SupportDefs.h>
#include <string>
#include <vector>
#include <Entry.h>

class LSPProjectWrapper;
class LSPTextDocument;
class BMessenger;
class BPath;
class BString;

class LSPServerConfigInterface {
public:
	virtual ~LSPServerConfigInterface() = default;
	virtual const bool   IsFileTypeSupported (const BString& fileType) const = 0;
	const char* const* Argv() const { return fArgv.data(); }
				int32  Argc() const { return fArgv.size(); }

protected:
	std::vector<const char*>	fArgv;
	int32 fOffset;
};

class LSPServersManager {
public:
		static status_t				InitLSPServersConfig();
		static LSPProjectWrapper*	CreateLSPProject(const BPath& path, const BMessenger& msgr, const BString& fileType);
		static status_t				DisposeLSPServersConfig();
private:
		static bool _AddValidConfig(LSPServerConfigInterface*);
		static std::vector<LSPServerConfigInterface*>	fConfigs;
};


#endif // LSPServersManager_H
