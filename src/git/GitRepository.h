/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Based on TrackGit (https://github.com/HaikuArchives/TrackGit)
 * Original author: Hrishikesh Hiraskar <hrishihiraskar@gmail.com>
 * Copyright Hrishikesh Hiraskar and other HaikuArchives contributors (see GitHub repo for details)
 */

#pragma once


#include <git2.h>

#include <functional>
#include <vector>

#include "GException.h"
#include "Log.h"


class BPath;

namespace Genio::Git {

	const int CANCEL_CREDENTIALS = -123;

	enum PullResult {
		UpToDate,
		FastForwarded,
		Merged
	};

	// Git exceptions

	class GitException : public GException {
	public:
		GitException(int error, BString const& message)
			:
			GException(error, message)
		{
			LogError("GitException %s, error = %d" , message.String(), error);
		}
	};


	class GitConflictException : public GitException {
	public:
		GitConflictException(int error, BString const& message,
			const std::vector<BString> files)
			:
			GitException(error, message),
			fFiles(files)
		{
			LogError("GitConflictException %s, error = %d, files = %d" , message.String(), error, files.size());
		}

		std::vector<BString>	GetFiles() const noexcept { return fFiles; }

	private:
		std::vector<BString> 	fFiles;
	};


	class GitRepository {
	public:
		typedef std::vector<std::pair<BString, BString>> RepoFiles;

		// Payload to search for merge branch.
		struct fetch_payload {
			char branch[100];
			git_oid branch_oid;
		};


										GitRepository(const BString path);
										~GitRepository();

		const BPath						Clone(const BString url, const BPath localPath,
												git_indexer_progress_cb progress_callback,
												git_credential_acquire_cb authentication_callback);

		static bool						IsValid(const BString path);
		bool							IsInitialized();
		void							Init(bool createInitalCommit = true);

		std::vector<BString>			GetTags() const;

		std::vector<BString>			GetBranches(git_branch_t type = GIT_BRANCH_LOCAL) const;
		int								SwitchBranch(const BString branch);
		BString							GetCurrentBranch() const;
		void							DeleteBranch(const BString branch, git_branch_t type);
		void							RenameBranch(const BString oldName, const BString newName,
											git_branch_t type);
		void							CreateBranch(const BString existingBranchName,
											git_branch_t type, const BString newBranchName);

		void							Fetch(bool prune = false);
		void							Merge(const BString source, const BString dest);
		PullResult						Pull(const BString branchName);
		void 							PullRebase();
		void 							Push();

		git_signature*					_GetSignature() const;

		void 							StashSave(const BString message);
		void 							StashPop();
		void 							StashApply();

		RepoFiles						GetFiles() const;

	private:
		git_repository 					*fRepository;
		BString							fRepositoryPath;
		bool							fInitialized;

		void							_Open();

		BString							_ConfigGet(git_config *cfg,
											const char *key) const;
		void							_ConfigSet(git_config *cfg,
											const char *key, const char* value);

		int 							check(int status,
											std::function<void(void)> execute_on_fail = nullptr,
											std::function<bool(const int)> custom_checker = nullptr) const;

		int 							_FastForward(const git_oid *target_oid, int is_unborn);
		int								_CreateCommit(git_index* index, const char* message);
		void							_CreateInitialCommit();
	};
}