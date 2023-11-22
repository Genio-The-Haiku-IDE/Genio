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

#include "GitCredentialsWindow.h"

namespace Genio::Git {

	GitRepository::GitRepository(const BString& path)
		:
		fRepository(nullptr),
		fRepositoryPath(path)
	{
		git_libgit2_init();
		git_buf buf = GIT_BUF_INIT_CONST(NULL, 0);
		if (git_repository_discover(&buf, fRepositoryPath.String(), 0, NULL) == 0) {
			check(git_repository_open(&fRepository, fRepositoryPath.String()));
		}
	}

	GitRepository::~GitRepository()
	{
		git_repository_free(fRepository);
		git_libgit2_shutdown();
	}

	vector<BString>
	GitRepository::GetBranches(git_branch_t branchType)
	{
		int error = 0;
		git_branch_iterator *it = nullptr;
		check(git_branch_iterator_new(&it, fRepository, branchType));

		vector<BString> branches;
		git_reference *ref = nullptr;
		git_branch_t type;
		while ((error = git_branch_next(&ref, &type, it)) == 0) {
			const char *branchName;
			error = git_branch_name(&branchName, ref);
			if (error < 0) {
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
		git_object* tree = nullptr;
		git_checkout_options opts;
		int status = 0;
		std::vector<std::string> files;
		BString remoteBranchName;

		check(git_checkout_init_options(&opts, GIT_CHECKOUT_OPTIONS_VERSION));

		opts.notify_flags =	GIT_CHECKOUT_NOTIFY_CONFLICT;
		opts.checkout_strategy = GIT_CHECKOUT_SAFE;
		opts.notify_cb = checkout_notify;
		opts.notify_payload = &files;

		check(git_revparse_single(&tree, fRepository, branchName.String()));

		// if the target branch is remote and a corresponding local branch does not exist, we need
		// to create a local branch first, set the remote tracking then checkout
		if (branchName.StartsWith("origin")) {
			remoteBranchName = branchName;
			branchName.RemoveFirst("origin/");

			git_reference* ref = nullptr;
			status = git_branch_create(&ref, fRepository, branchName.String(), (git_commit*)tree, false);
			if (status >= 0) {
				check(git_branch_set_upstream(ref, remoteBranchName.String()));
				git_reference_free(ref);
			}
		}

		check(git_checkout_tree(fRepository, tree, &opts));

		BString ref("refs/heads/%s");
		branchName.RemoveFirst("origin/");
		ref.ReplaceFirst("%s", branchName.String());
		check(git_repository_set_head(fRepository, ref.String()));

		return status;
	}

	BString
	GitRepository::GetCurrentBranch()
	{
		int error = 0;
		const char *branch = nullptr;
		git_reference *head = nullptr;

		error = git_repository_head(&head, fRepository);

		if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND)
			branch = nullptr;
		else if (!error) {
			branch = git_reference_shorthand(head);
		} else {
			throw GitException(error, git_error_last()->message);
		}

		BString branchText((branch) ? branch : "");
		git_reference_free(head);
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

		check(git_status_list_new(&status, fRepository, &statusopt));

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
			// TODO: Change to GitException and maybe refactor inheriting from std:exception
			if (error == CANCEL_CREDENTIALS)
				throw GitException(error, "CANCEL_CREDENTIALS");
			else
				throw GitException(error, git_error_last()->message);
		}
		return localPath;
	}

	void
	GitRepository::Fetch(bool prune)
	{
		git_remote *remote;
		git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
		fetch_opts.callbacks.credentials = GitCredentialsWindow::authentication_callback;
		if (prune)
			fetch_opts.prune = GIT_FETCH_PRUNE;
		else
			fetch_opts.prune = GIT_FETCH_NO_PRUNE;

		/* lookup the remote */
		check(git_remote_lookup(&remote, fRepository, "origin"));
		check(git_remote_fetch(remote, nullptr, &fetch_opts, nullptr));
	}

	void
	GitRepository::Merge(BString &source, BString &dest)
	{
	}

	void
	GitRepository::Pull(bool rebase)
	{
	}

	void
	GitRepository::Push()
	{
	}

	BString
	GitRepository::_ConfigGet(git_config *cfg, const char *key)
	{

	  git_config_entry *entry;
	  BString ret("");

	  check(git_config_get_entry(&entry, cfg, key),
			[](const int x) { return (x != GIT_ENOTFOUND); });

	  ret = entry->value;

	  git_config_entry_free(entry);

	  return ret;
	}

	void
	GitRepository::_ConfigSet(git_config *cfg, const char *key, const char *value)
	{
		check(git_config_set_string(cfg, key, value));
	}

	int
	GitRepository::check(const int status, std::function<bool(const int)> checker)
	{
		if (checker != nullptr) {
			if (checker(status))
				throw GitException(status, git_error_last()->message);
		} else {
			if (status < 0)
				throw GitException(status, git_error_last()->message);
		}
		return status;
	}

	void
	GitRepository::StashSave()
	{
		git_oid saved_stash;
		git_signature *signature;
		int status;

		// create a test signature
		// TODO - Put this into Genio Prefs
		git_signature_new(&signature, "no name", "no.name@gmail.com", 1323847743, 60);

		// TODO - We need a BAlert like requester?
		BString stash_message;
		stash_message << "WIP on " << GetCurrentBranch() << B_UTF8_ELLIPSIS;
		status = git_stash_save(&saved_stash, fRepository, signature,
				   stash_message.String(), /*GIT_STASH_INCLUDE_UNTRACKED*/0);
		if (status < 0) {
			git_signature_free(signature);
			throw GitException(status, git_error_last()->message);
		}

		// free signature
		git_signature_free(signature);
	}

	void
	GitRepository::StashPop()
	{
		git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;
		check(git_stash_pop(fRepository, 0, &opts));
	}

	void
	GitRepository::StashApply()
	{
		git_stash_apply_options opts = GIT_STASH_APPLY_OPTIONS_INIT;
		check(git_stash_apply(fRepository, 0, &opts));
	}


	vector<BString>
	GitRepository::GetTags()
	{
		vector<BString> tags;

		auto lambda = [](git_reference *ref, void *payload) -> int
		{
			auto tags = reinterpret_cast<vector<BString>*>(payload);
			BString ref_name(git_reference_name(ref));
			if (!ref_name.IFindFirst("refs/tags/")) {
				ref_name.RemoveAll("refs/tags/");
				tags->push_back(ref_name);
				// return 1;
			}
			return 0;
		};
		git_reference_foreach(fRepository, lambda, &tags);

		return tags;
	}

}