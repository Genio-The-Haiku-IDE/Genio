/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Based on TrackGit (https://github.com/HaikuArchives/TrackGit)
 * Original author: Hrishikesh Hiraskar <hrishihiraskar@gmail.com> 
 * Copyright Hrishikesh Hiraskar and other HaikuArchives contributors (see GitHub repo for details)
 */


#include "GitRepository.h"

#include <map>

namespace Genio::Git {

	std::map<float,bool> progress_tracker;

	GitRepository::GitRepository(const path& path)
		: 
		fRepository(nullptr),
		fRepositoryPath(path),
		fInitCheck(false)
	{
		git_libgit2_init();
		git_buf buf = GIT_BUF_INIT_CONST(NULL, 0);
		if (git_repository_discover(&buf, path.c_str(), 0, NULL) == 0) {
			int error = git_repository_open(&fRepository, path.c_str());
			if (error != 0) {
				throw GitException(error, git_error_last()->message);
			}
			fInitCheck = true;
		}
	}

	GitRepository::~GitRepository()
	{
		git_repository_free(fRepository);
		git_libgit2_shutdown();
	}
	
	vector<string>
	GitRepository::GetBranches()
	{
		git_branch_iterator *it;
		git_reference *ref;
		vector<string> branches;

		int error = git_branch_iterator_new(&it, fRepository, GIT_BRANCH_LOCAL);
		if (error != 0) {
			throw GitException(error, git_error_last()->message);
		}

		while ((error = git_branch_next(&ref, NULL, it)) == 0) {
			const char *branchName;
			error = git_branch_name(&branchName, ref);
			if (error != 0) {
				git_reference_free(ref);
				throw GitException(error, git_error_last()->message);
			}
			branches.push_back(branchName);
			git_reference_free(ref);
		}

		git_branch_iterator_free(it);

		return branches;
	}

	vector<pair<string, string>>
	GitRepository::GetFiles() 
	{
		git_status_options statusopt;
		git_status_list *status;
		size_t i, maxi;
		const git_status_entry *s;

		git_status_init_options(&statusopt, GIT_STATUS_OPTIONS_VERSION);
		statusopt.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
		statusopt.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

		int error = git_status_list_new(&status, fRepository, &statusopt);
		if (error != 0) {
			throw GitException(error, git_error_last()->message);
		}

		vector<pair<string, string>> fileStatuses;

		maxi = git_status_list_entrycount(status);
		for (i = 0; i < maxi; ++i) {
			s = git_status_byindex(status, i);

			if (s->status == GIT_STATUS_CURRENT)
				continue;

			string filePath = s->index_to_workdir->old_file.path;
			string fileStatus;

			if (s->status & GIT_STATUS_WT_NEW)
				fileStatus = B_TRANSLATE("New file");

			else if (s->status & GIT_STATUS_INDEX_NEW)
				fileStatus = B_TRANSLATE("New file staged");

			else if (s->status & GIT_STATUS_WT_MODIFIED)
				fileStatus = B_TRANSLATE("File modified");

			else if (s->status & GIT_STATUS_INDEX_MODIFIED)
				fileStatus = B_TRANSLATE("File modified on index");

			else if (s->status & GIT_STATUS_WT_DELETED)
				fileStatus = B_TRANSLATE("File deleted");

			else if (s->status & GIT_STATUS_INDEX_DELETED)
				fileStatus = B_TRANSLATE("File deleted from index");

			fileStatuses.emplace_back(filePath, fileStatus);
		}

		git_status_list_free(status);

		return fileStatuses;
	}

	string
	GitRepository::_ExecuteCommand(const string& command) const 
	{
		char buffer[128];
		string result = "";
		unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
		if (!pipe) {
			throw runtime_error("popen() failed!");
		}
		while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr) {
			result += buffer;
		}
		return result;
	}

	string
	GitRepository::PullChanges(bool rebase)
	{
		string pullCommand = "git -C " + fRepositoryPath.string() + " pull";
		if (rebase) {
			pullCommand += " --rebase";
		}
		return _ExecuteCommand(pullCommand);
	}
	
	void
	GitRepository::Clone(const string& url, const path& localPath)
	{
		auto thread = new CloneThread(url, localPath);
		thread->Start();
	}
	
}