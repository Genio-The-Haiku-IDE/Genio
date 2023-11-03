/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Based on TrackGit (https://github.com/HaikuArchives/TrackGit)
 * Original author: Hrishikesh Hiraskar <hrishihiraskar@gmail.com>
 * Copyright Hrishikesh Hiraskar and other HaikuArchives contributors (see GitHub repo for details)
 */


#include "GitRepository.h"

#include <Application.h>
#include <Path.h>

#include <stdexcept>
#include <map>

namespace Genio::Git {

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

	vector<BString>
	GitRepository::GetBranches()
	{
		git_branch_iterator *it;
		git_reference *ref;
		git_branch_t type;
		vector<BString> branches;

		int error = git_branch_iterator_new(&it, fRepository, GIT_BRANCH_ALL);
		if (error != 0) {
			throw GitException(error, git_error_last()->message);
		}

		while ((error = git_branch_next(&ref, &type, it)) == 0) {
			const char *branchName;
			error = git_branch_name(&branchName, ref);
			if (error != 0) {
				git_reference_free(ref);
				throw GitException(error, git_error_last()->message);
			}
			BString bBranchName(branchName);
			if (bBranchName.FindFirst("HEAD") == B_ERROR)
				branches.push_back(branchName);
			git_reference_free(ref);
		}

		git_branch_iterator_free(it);

		return branches;
	}


	int
	checkout_notify(git_checkout_notify_t why, const char *path,
									const git_diff_file *baseline,
									const git_diff_file *target,
									const git_diff_file *workdir,
									void *payload)
	{
		std::vector<std::string> *files = reinterpret_cast<std::vector<std::string>*>(payload);

		BString temp;
		temp.SetToFormat("'%s' - ", path);
		switch (why) {
			case GIT_CHECKOUT_NOTIFY_CONFLICT:
				files->push_back(path);
				temp.Append("conflict\n");
			break;
			case GIT_CHECKOUT_NOTIFY_DIRTY:
			break;
			case GIT_CHECKOUT_NOTIFY_UPDATED:
			break;
			case GIT_CHECKOUT_NOTIFY_UNTRACKED:
			break;
			case GIT_CHECKOUT_NOTIFY_IGNORED:
			break;
			default:
			break;
		}

		LogInfo(temp.String());
		return 0;
	}

	int
	GitRepository::SwitchBranch(BString &branchName)
	{
		git_object* tree = NULL;
		git_checkout_options opts;
		int status = 0;
		std::vector<std::string> files;

		status = git_checkout_init_options(&opts, GIT_CHECKOUT_OPTIONS_VERSION);
		if (status < 0)
			throw GitException(status, git_error_last()->message);

		opts.notify_flags =	GIT_CHECKOUT_NOTIFY_CONFLICT;
		opts.checkout_strategy = GIT_CHECKOUT_SAFE;
		opts.notify_cb = checkout_notify;
		opts.notify_payload = &files;

		status = git_revparse_single(&tree, fRepository, branchName.String());
		if (status < 0)
			throw GitException(status, git_error_last()->message);

		status = git_checkout_tree(fRepository, tree, &opts);
		if (status < 0) {
			throw GitException(status, git_error_last()->message, files);
		}

		BString ref("refs/heads/%s");
		branchName.RemoveFirst("origin/");
		ref.ReplaceFirst("%s", branchName.String());
		status = git_repository_set_head(fRepository, ref.String());
		if (status < 0)
			throw GitException(status, git_error_last()->message);

		return status;
	}

	BString
	GitRepository::GetCurrentBranch()
	{
		int error = 0;
		const char *branch = NULL;
		git_reference *head = NULL;

		error = git_repository_head(&head, fRepository);

		if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND)
			branch = NULL;
		else if (!error) {
			branch = git_reference_shorthand(head);
		} else {
			throw GitException(error, git_error_last()->message);
		}

		git_reference_free(head);

		BString branchText((branch) ? branch : "");
		return branchText;
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

	const BPath&
	GitRepository::Clone(const string& url, const BPath& localPath,
							git_indexer_progress_cb callback,
							git_credential_acquire_cb authentication_callback)
	{
		git_libgit2_init();
		git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
		git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
		callbacks.transfer_progress = callback;
		clone_opts.fetch_opts.callbacks = callbacks;
		clone_opts.fetch_opts.callbacks.credentials = authentication_callback;

		git_repository *repo = nullptr;

		int error = git_clone(&repo, url.c_str(), localPath.Path(), &clone_opts);
		if (error < 0) {
			if (error == CANCEL_CREDENTIALS)
				throw std::runtime_error("CANCEL_CREDENTIALS");
			else
				throw std::runtime_error(git_error_last()->message);
		}
		return localPath;
	}

}