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

#include <cstdio>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

#include "Utils.h"
#include "Log.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GitRepository"

using namespace std;
using namespace std::filesystem;

namespace Genio::Git {

	const int CANCEL_CREDENTIALS = -123;

	class GitException {
	public:
									GitException(int error, BString const& message)
										:
										_error(error),
										_message(message)
									{
										LogError("GitException %s, error = %d" , message.String(), error);
									}

									GitException(int error, BString const& message,
													std::vector<std::string> files)
										:
										_error(error),
										_message(message),
										_files(files)
									{
										LogError("GitException %s, error = %d" , message.String(), error);
									}

		int							Error() noexcept { return _error; }
		BString						Message() noexcept { return _message; }
		std::vector<std::string>	GetFiles() noexcept { return _files; }

	private:
		int							_error;
		BString						_message;
		std::vector<std::string> 	_files;
	};

	class GitRepository {
	public:

											GitRepository(const path& path);
											~GitRepository();

		const BPath&						Clone(const string& url, const BPath& localPath,
													git_indexer_progress_cb callback,
													git_credential_acquire_cb authentication_callback);

		bool								InitCheck() { return fInitCheck; };

		void								Init(const path& repo, const path& localPath);

		vector<BString>						GetBranches(git_branch_t type = GIT_BRANCH_LOCAL);
		int									SwitchBranch(BString &branch);
		BString								GetCurrentBranch();

		void								Fetch(bool prune = false);
		void								Merge(BString &source, BString &dest);
		void 								Pull(bool rebase = false);
		void 								Push();

		void 								StashSave();
		void 								StashPop();
		void 								StashApply();

		vector<pair<string, string>>		GetFiles();

	private:

		git_repository 						*fRepository;
		path 								fRepositoryPath;
		bool								fInitCheck;

		BString								_ConfigGet(git_config *cfg,
														const char *key);
		void								_ConfigSet(git_config *cfg,
														const char *key, const char* value);

		int 								check(int status,
													std::function<bool(const int)> checker = nullptr);

	};

}