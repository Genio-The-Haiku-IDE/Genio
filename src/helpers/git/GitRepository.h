/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Based on TrackGit (https://github.com/HaikuArchives/TrackGit)
 * Original author: Hrishikesh Hiraskar <hrishihiraskar@gmail.com> 
 * Copyright Hrishikesh Hiraskar and other HaikuArchives contributors (see GitHub repo for details)
 */
 
#pragma once

#include <SupportDefs.h>
#include <Catalog.h>
#include <Notification.h>

#include <git2.h>

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <cstdio>
#include <memory>

#include "Utils.h"
#include "CloneThread.h"
#include "Log.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GitRepository"

using namespace std;
using namespace std::filesystem;

namespace Genio::Git {

	class GitException {
	public:
						GitException(int error, string const& message)
							:
							_error(error),
							_message(message)
						{
							LogError("GitException %s, error = %d" , message, error);
						}

		int				Error() noexcept { return _error; }
		string			Message() noexcept { return _message; }
				
	private:
		int				_error;
		string			_message;
	};

	class GitRepository {	
	public:
											GitRepository(const path& path);
											~GitRepository();
											
		static void						Clone(const string& url, const path& localPath);
		
		bool								InitCheck() { return fInitCheck; };
		void								Init(const path& repo, const path& localPath);

		vector<string> 						GetBranches();
		int									SwitchBranch(string branch);
		
		vector<pair<string, string>>		GetFiles();
		
		string 								PullChanges(bool rebase);

	private:
		git_repository 						*fRepository;
		path 								fRepositoryPath;
		bool								fInitCheck;
		
		string 								_ExecuteCommand(const string& command) const;
		
		static int 							_CloneProgressCallback(const git_transfer_progress *stats, 
																	void *payload);
	};
	
}