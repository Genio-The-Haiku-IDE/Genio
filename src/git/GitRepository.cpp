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
#include "GTextAlert.h"

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

		status = git_checkout_tree(fRepository, tree, &opts);
		if (status < 0) {
			throw GitException(status, git_error_last()->message, files);
		}

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

	void
	GitRepository::DeleteBranch(BString &branch, git_branch_t type)
	{
		git_reference* ref = nullptr;
		check(git_branch_lookup(&ref, fRepository, branch.String(), type));
		check(git_branch_delete(ref));
		git_reference_free(ref);
	}

	void
	GitRepository::RenameBranch(BString &old_name, BString &new_name, git_branch_t type)
	{
		git_reference* ref = nullptr;
		git_reference* out = nullptr;
		check(git_branch_lookup(&ref, fRepository, old_name.String(), type));
		check(git_branch_move(&out, ref, new_name.String(), 0));
		git_reference_free(ref);
		git_reference_free(out);
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

		git_remote_free(remote);
	}

	void
	GitRepository::Merge(BString &source, BString &dest)
	{
	}

	PullResult
	GitRepository::Pull()
	{
		git_remote *remote = nullptr;
		git_annotated_commit* heads[1];

		auto CleanUp = [this, &heads, &remote]() -> void {
			git_annotated_commit_free(heads[0]);
			git_repository_state_cleanup(fRepository);
			git_remote_free(remote);
		};

		// Fetch the changes from the remote
		Fetch();

		auto fetchhead_ref_cb = [](const char* name, const char* url, const git_oid* oid,
			unsigned int is_merge, void* payload_v) -> int
		{
			struct fetch_payload* payload = (struct fetch_payload*) payload_v;
			if (is_merge) {
				strcpy(payload->branch, name);
				memcpy(&payload->branch_oid, oid, sizeof(git_oid));
			}
			return 0;
		};

		struct fetch_payload payload;
		git_repository_fetchhead_foreach(fRepository, fetchhead_ref_cb, &payload);
		check(git_annotated_commit_lookup(&heads[0], fRepository, &payload.branch_oid),	CleanUp);

		// TODO: REBASE:git_rebase_init, git_rebase_next, git_rebase_commit, git_rebase_finish

		// Perform a merge operation
		git_merge_analysis_t merge_analysis_t;
		git_merge_preference_t merge_preference_t;
		check(git_merge_analysis(&merge_analysis_t, &merge_preference_t,
								 fRepository, (const git_annotated_commit**)&heads[0], 1), CleanUp);

		if (merge_analysis_t & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
			CleanUp();
			return PullResult::UpToDate;
		} else if (merge_analysis_t & GIT_MERGE_ANALYSIS_FASTFORWARD) {
			check(_FastForward(&payload.branch_oid,	(merge_analysis_t & GIT_MERGE_ANALYSIS_UNBORN)),
				CleanUp);
			return PullResult::FastForwarded;
		} else if (merge_analysis_t & GIT_MERGE_ANALYSIS_NORMAL) {
			git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
			git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

			merge_opts.file_flags = GIT_MERGE_FILE_STYLE_DIFF3;

			checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_ALLOW_CONFLICTS;

			check(git_merge(fRepository,
							(const git_annotated_commit **)&heads[0], 1,
							&merge_opts, &checkout_opts), CleanUp);
		}

		/* If we get here, we actually performed the merge above */
		git_index* index;
		check(git_repository_index(&index, fRepository));
		if (!git_index_has_conflicts(index))
			check(_CreateCommit(index, "Merge"));
		git_index_free(index);
		CleanUp();
		return PullResult::Merged;
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

	  check(git_config_get_entry(&entry, cfg, key), nullptr,
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
	GitRepository::check(const int status,
		std::function<void(void)> execute_on_fail,
		std::function<bool(const int)> checker)
	{
		if (checker != nullptr) {
			if (checker(status)) {
				if (execute_on_fail != nullptr)
					execute_on_fail();
				throw GitException(status, git_error_last()->message);
			}
		} else {
			if (status < 0) {
				if (execute_on_fail != nullptr)
					execute_on_fail();
				throw GitException(status, git_error_last()->message);
			}
		}
		return status;
	}

	void
	GitRepository::StashSave(const BString &message)
	{
		git_oid saved_stash;
		git_signature *signature;

		// TODO - Put this into Genio Prefs
		git_config* cfg;
		git_config* cfg_snapshot;

		git_config_open_ondisk(&cfg, "/boot/home/config/settings/git/config");
		git_config_snapshot(&cfg_snapshot, cfg);

		const char *user_name, *user_email;
		check(git_config_get_string(&user_name, cfg_snapshot, "user.name"));
		check(git_config_get_string(&user_email, cfg_snapshot, "user.email"));
		check(git_signature_now(&signature, user_name, user_email));

		auto CleanUp = [signature]() { git_signature_free(signature); };

		// TODO: consider using GIT_STASH_INCLUDE_UNTRACKED
		check(git_stash_save(&saved_stash, fRepository, signature,
				   message.String(), 0),
				   CleanUp);

		CleanUp();
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

	int
	GitRepository::_FastForward(const git_oid *target_oid, int is_unborn)
	{
		git_checkout_options ff_checkout_options = GIT_CHECKOUT_OPTIONS_INIT;
		git_reference *target_ref;
		git_reference *new_target_ref;
		git_object *target = NULL;
		int err = 0;

		if (is_unborn) {
			const char *symbolic_ref;
			git_reference *head_ref;

			// HEAD reference is unborn, lookup manually so we don't try to resolve it
			check(git_reference_lookup(&head_ref, fRepository, "HEAD"));

			// Grab the reference HEAD should be pointing to
			symbolic_ref = git_reference_symbolic_target(head_ref);

			// Create our master reference on the target OID
			check(git_reference_create(&target_ref, fRepository, symbolic_ref,
					target_oid, 0, NULL));

			git_reference_free(head_ref);
		} else {
			// HEAD exists, just lookup and resolve
			check(git_repository_head(&target_ref, fRepository));
		}

		// Lookup the target object
		check(git_object_lookup(&target, fRepository, target_oid, GIT_OBJ_COMMIT));

		// Checkout the result so the workdir is in the expected state
		std::vector<std::string> files;
		ff_checkout_options.notify_flags =	GIT_CHECKOUT_NOTIFY_CONFLICT;
		ff_checkout_options.notify_cb = checkout_notify;
		ff_checkout_options.notify_payload = &files;
		ff_checkout_options.checkout_strategy = GIT_CHECKOUT_SAFE;
		int status = git_checkout_tree(fRepository, target, &ff_checkout_options);
		if (status < 0) {
			throw GitException(status, git_error_last()->message, files);
		}

		// Move the target reference to the target OID
		check(git_reference_set_target(&new_target_ref, target_ref, target_oid,	NULL));

		git_reference_free(target_ref);
		git_reference_free(new_target_ref);
		git_object_free(target);

		return err;
	}

	int
	GitRepository::_CreateCommit(git_index* index, const char* message)
	{
		git_tree* tree;
		git_commit* parent;
		git_oid tree_id, commit_id, parent_id;
		git_signature* sign;
		git_config* cfg;
		git_config* cfg_snapshot;
		int ret;

		git_config_open_ondisk(&cfg, "/boot/home/config/settings/git/config");
		git_config_snapshot(&cfg_snapshot, cfg);

		const char *user_name, *user_email;
		ret = git_config_get_string(&user_name, cfg_snapshot, "user.name");
		if (ret < 0)
			return ret;

		ret = git_config_get_string(&user_email, cfg_snapshot, "user.email");
		if (ret < 0)
			return ret;

		// Get default commiter
		ret = git_signature_now(&sign, user_name, user_email);
		if (ret < 0)
			return ret;

		// Init tree_id
		ret = git_index_write_tree(&tree_id, index);
		if (ret < 0)
			return ret;

		git_index_free(index);

		// Init tree
		ret = git_tree_lookup(&tree, fRepository, &tree_id);
		if (ret < 0)
			return ret;

		ret = git_reference_name_to_id(&parent_id, fRepository, "HEAD");
		if (ret < 0) {
			// No parent commit found. This might be initial commit.
			ret = git_commit_create_v(&commit_id, fRepository, "HEAD", sign, sign, NULL,
					message, tree, 0);
			return ret;
		}

		ret = git_commit_lookup(&parent, fRepository, &parent_id);
		if (ret < 0)
			return ret;

		// Create commit
		ret = git_commit_create_v(&commit_id, fRepository, "HEAD", sign, sign, NULL,
				message, tree, 1, parent);
		return ret;
	}

}