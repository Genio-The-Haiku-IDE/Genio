/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "LSPServersManager.h"

#include "ConfigManager.h"
#include "GenioApp.h"
#include "Log.h"
#include "LSPLogLevels.h"
#include "LSPProjectWrapper.h"

#include <vector>
#include <string>


class ClangdServerConfig : public LSPServerConfigInterface {
public:
	ClangdServerConfig()
	{
		std::string logLevel("--log=");
		switch ((int32) gCFG["lsp_clangd_log_level"]) {
			default:
			case LSP_LOG_LEVEL_ERROR:
				logLevel += "error"; // Error messages only
				break;
			case LSP_LOG_LEVEL_INFO:
				logLevel += "info"; // High level execution tracing
				break;
			case LSP_LOG_LEVEL_TRACE:
				logLevel += "verbose"; // Low level details
				break;
		};

		fArgv = {
			"/boot/system/bin/clangd",
			strdup(logLevel.c_str()),
			"--offset-encoding=utf-8",
			"--pretty",
			"--header-insertion-decorators=false",
			"--pch-storage=memory"
		};

		fOffset = 1;
	}

	const bool	IsFileTypeSupported(const BString& fileType) const {
		if (fileType.Compare("cpp") != 0 &&
			fileType.Compare("c") != 0 &&
			fileType.Compare("makefile") != 0)
			return false;
		return true;
	}
};

class PylspServerConfig : public LSPServerConfigInterface {
public:
	PylspServerConfig() {
		fArgv = {
			"/boot/system/non-packaged/bin/pylsp",
			"-v"
		};
	}
	const bool	IsFileTypeSupported(const BString& fileType) const {
		return (fileType.Compare("python") == 0);
	}
};

class OmniSharpServerConfig : public LSPServerConfigInterface {
public:
	OmniSharpServerConfig() {
		thread_id pid = find_thread(NULL);
		BString spid;
		spid << (int32)pid;
		fArgv = {
			"/boot/system/non-packaged/bin/dotnet/dotnet",
			"/boot/system/non-packaged/bin/OmniSharp/OmniSharp.dll",
			"-lsp",
			"-v",
			"--hostPID",
			strdup(spid.String())
		};

		fOffset = 0;
	}
	const bool	IsFileTypeSupported(const BString& fileType) const {
		return (fileType.Compare("csharp") == 0);
	}
};

// Experimental config for csharp-language-server by razzmatazz
// class CSharpLanguageServerConfig : public LSPServerConfigInterface {
// public:
	// CSharpLanguageServerConfig() {
		// fArgv = {
			// "/boot/system/non-packaged/bin/dotnet/dotnet",
			// "/boot/home/workspace/csharp-language-server/src/CSharpLanguageServer/bin/Debug/net8.0/CSharpLanguageServer.dll",
			// "-s",
			// "##SOLUTION##"
		// };
	// }
	// const bool	IsFileTypeSupported(const BString& fileType) const {
		// return (fileType.Compare("cs") != 0 &&
				// fileType.Compare("csproj") != 0 &&
				// fileType.Compare("sln") != 0 &&
				// fileType.Compare("cake") != 0);
	// }
// };



std::vector<LSPServerConfigInterface*> LSPServersManager::fConfigs;

/*static*/
bool
LSPServersManager::_AddValidConfig(LSPServerConfigInterface* interface)
{
	if (interface->Argc() > 0 && BEntry(interface->Argv()[0], true).Exists()) {
		fConfigs.push_back(interface);
		return true;
	}
	LogInfo("LSP Server [%s] not installed!", interface->Argv()[0]);
	return false;
}

/*static*/
status_t
LSPServersManager::InitLSPServersConfig()
{
	_AddValidConfig(new ClangdServerConfig());
	_AddValidConfig(new PylspServerConfig());
	_AddValidConfig(new OmniSharpServerConfig());
	return B_OK;
}

/*static*/
status_t
LSPServersManager::DisposeLSPServersConfig()
{
	for (LSPServerConfigInterface* interface: fConfigs) {
		delete interface;
	}
	fConfigs.clear();
	return B_OK;
}

/*static*/
LSPProjectWrapper*
LSPServersManager::CreateLSPProject(const BPath& path, const BMessenger& msgr, const BString& fileType)
{
	for (LSPServerConfigInterface* interface: fConfigs) {
		if (interface->IsFileTypeSupported(fileType)) {
			return new LSPProjectWrapper(path, msgr, *interface);
		}
	}
	return nullptr;
}