/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "LSPServersManager.h"
#include "LSPLogLevels.h"
#include "ConfigManager.h"
#include "LSPProjectWrapper.h"
#include "GenioApp.h"
#include <vector>
#include <string>



class ClangdServer : public LSPServerConfigInterface {
public:
	ClangdServer()
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
			"clangd",
			strdup(logLevel.c_str()),
			"--offset-encoding=utf-8",
			"--pretty",
			"--header-insertion-decorators=false",
			"--pch-storage=memory"
		};
	}

	const bool	IsFileTypeSupported(const BString& fileType) const {
		if (fileType.Compare("cpp")  != 0 &&
			fileType.Compare("c")    != 0 &&
			fileType.Compare("makefile") != 0)
			return false;
		return true;
	}
};

class PylspServer : public LSPServerConfigInterface {
public:
	PylspServer() {
		fArgv = {
			"/boot/system/non-packaged/bin/pylsp",
			"-v"
		};
	}
	const bool	IsFileTypeSupported(const BString& fileType) const {
		return (fileType.Compare("python")  == 0);
	}
};

std::vector<LSPServerConfigInterface*> gLSPServers ({new ClangdServer()/*, new PylspServer()*/});

LSPProjectWrapper*
LSPServersManager::CreateLSPProject(const BPath& path, const BMessenger& msgr, const BString& fileType)
{
	for (LSPServerConfigInterface* interface: gLSPServers) {
		if (interface->IsFileTypeSupported(fileType)) {
			return new LSPProjectWrapper(path, msgr, *interface);
		}
	}
	return nullptr;
}