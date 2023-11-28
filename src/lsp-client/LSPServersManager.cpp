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
	const bool	IsFileTypeSupported(const BString& fileType) const {
		if (fileType.Compare("cpp")  != 0 &&
			fileType.Compare("c")    != 0 &&
			fileType.Compare("makefile") != 0)
			return false;
		return true;
	}

	const char** CreateServerStartupArgs(int32& argc) const {

			///configuration for clangd
			std::string logLevel("--log=");
			switch ((int32)gCFG["lsp_clangd_log_level"]) {
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
			const char** argv = new const char*[7];
			argv[0] = strdup("clangd");
			argv[1] = strdup(logLevel.c_str());
			argv[2] = strdup("--offset-encoding=utf-8");
			argv[3] = strdup("--pretty");
			argv[4] = strdup("--header-insertion-decorators=false");
			argv[5] = strdup("--pch-storage=memory");
			argv[6] = nullptr;

			argc = 6;
			return argv;
	}
};

class PylspServer : public LSPServerConfigInterface {
public:
	const bool	IsFileTypeSupported(const BString& fileType) const {
		return (fileType.Compare("python")  == 0);
	}

	const char** CreateServerStartupArgs(int32& argc) const {
			const char** argv = new const char*[3];
			argv[0] = strdup("/boot/system/non-packaged/bin/pylsp");
			argv[1] = strdup("-v");
			argv[2] = nullptr;
			argc = 2;
			return argv;
	}

};


std::vector<LSPServerConfigInterface*> gLSPServers ({new ClangdServer(), new PylspServer()});

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