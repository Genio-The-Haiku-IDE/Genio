/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 * Based on TrackGit (https://github.com/HaikuArchives/TrackGit)
 * Original author: Hrishikesh Hiraskar 
 * Copyright Hrishikesh Hiraskar and other HaikuArchives contributors (see GitHub repo for details)
 */


#include "GitRepository.h"

#include <Application.h>
#include <Path.h>

#include "GitCredentialsWindow.h"

namespace Genio::Git {

	GitRepository::GitRepository(BString path)
		:
		fRepository(nullptr),
		fRepositoryPath(path),
		fInitialized(false)
	{
		git_libgit2_init();
		_Open();
	}

	GitRepository::~GitRepository()
	{
		git_repository_free(fRepository);
		git_libgit2_shutdown();
	}

	void
	GitRepository::_Open()
	{
		int status = 0;
		git_buf buf = GIT_BUF_INIT_CONST(NULL, 0);
		if (git_repository_discover(&buf, fRepositoryPath.String(), 0, NULL) >= 0) {
			status = git_repository_open(&fRepository, fRepositoryPath.String());
			if (status >= 0)
				fInitialized = true;
		}
	}

	bool
	GitRepository::IsInitialized()
	{
		// if the repository gets corrupted or the .git folder is deleted, we need to check the
		// status again and put the repository in an uninitialized state
		if (!GitRepository::IsValid(fRepositoryPath)) {
			fInitialized = false;
			// Let's make an attempt to reopen the repository if necessary
			_Open();
		}
		return fInitialized;
	}

	bool
	GitRepository::IsValid(BString path)
	{
		if (path == "")	{
			return false;
		}

		if (git_repository_open_ext(nullptr, path, GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr) < 0)
			return false;

		return true;
	}

	void
	GitRepository::Init(bool createInitalCommit)
	{
		check(git_repository_init(&fRepository, fRepositoryPath.String(), 0));
		if (createInitalCommit)
			_CreateInitialCommit();
		fInitialized = true;
	}

	std::vector<BString>
	GitRepository::GetBranches(git_branch_t branchType)
	{
		git_branch_iterator *it = nullptr;
		git_reference *ref = nullptr;

		try {
			check(git_branch_iterator_new(&it, fRepository, branchType));

			git_branch_t type;
			std::vector<BString> branches;
			while (git_branch_next(&ref, &type, it) == 0) {
				const char *branchName;
				check(git_branch_name(&branchName, ref));
				BString bBranchName(branchName);
				if (bBranchName.FindFirst("HEAD") == B_ERROR)
					branches.push_back(branchName);
			}

			git_reference_free(ref);
			git_branch_iterator_free(it);
			return branches;
		} catch (const GitException &e) {
			if (ref != nullptr) git_reference_free(ref);
			if (it != nullptr) git_branch_iterator_free(it);
			throw;
		}
	}


	int
	checkout_notify(git_checkout_notify_t why, const char *path,
									const git_diff_file *baseline,
									const git_diff_file *target,
									const git_diff_file *workdir,
									void *payload)
	{
		std::vector<BString> *files = reinterpret_cast<std::vector<BString>*>(payload);

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
	GitRepository::SwitchBranch(BString branchName)
	{
		git_object* tree = nullptr;
		git_reference* ref = nullptr;

		try {
			int status = 0;
			std::vector<BString> files;
			BString remoteBranchName;

			git_checkout_options opts;
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

				status = git_branch_create(&ref, fRepository, branchName.String(), (git_commit*)tree, false);
				if (status >= 0) {
					check(git_branch_set_upstream(ref, remoteBranchName.String()));
				}
			}

			status = git_checkout_tree(fRepository, tree, &opts);
			if (status < 0) {
				throw GitConflictException(status, git_error_last()->message, files);
			}

			BString sref("refs/heads/%s");
			branchName.RemoveFirst("origin/");
			sref.ReplaceFirst("%s", branchName.String());
			check(git_repository_set_head(fRepository, sref.String()));

			git_reference_free(ref);
			return status;
		} catch (const GitException &e) {
			if (ref != nullptr) git_reference_free(ref);
			throw;
		}
	}

	BString
	GitRepository::GetCurrentBranch()
	{
		const char *branch = nullptr;
		git_reference *head = nullptr;

		try {
			check(git_repository_head(&head, fRepository));
			branch = git_reference_shorthand(head);
			BString branchText((branch) ? branch : "");
			git_reference_free(head);
			return branchText;
		} catch (const GitException &e) {
			if (head != nullptr) git_reference_free(head);
			throw;
		}
	}

	void
	GitRepository::DeleteBranch(BString branch, git_branch_t type)
	{
		git_reference* ref = nullptr;
		try {
			check(git_branch_lookup(&ref, fRepository, branch.String(), type));
			check(git_branch_delete(ref));
			git_reference_free(ref);
		} catch (const GitException &e) {
			if (ref != nullptr) git_reference_free(ref);
			throw;
		}
	}

	void
	GitRepository::RenameBranch(BString old_name, BString new_name, git_branch_t type)
	{
		git_reference* ref = nullptr;
		git_reference* out = nullptr;
		try {
			check(git_branch_lookup(&ref, fRepository, old_name.String(), type));
			check(git_branch_move(&out, ref, new_name.String(), 0));
			git_reference_free(ref);
			git_reference_free(out);
		} catch (const GitException &e) {
			// Clean up in case of an exception
			if (ref != nullptr) git_reference_free(ref);
			if (out != nullptr) git_reference_free(out);

			// Re-throw the exception
			throw;
		}
	}

	void
	GitRepository::CreateBranch(BString existingBranchName, git_branch_t type, BString newBranchName)
	{
		git_reference *existing_branch_ref = nullptr;
		git_reference *new_branch_ref = nullptr;
		git_commit *commit = nullptr;

		try {
			// Lookup the existing branch
			int status = 0;
			status = git_branch_lookup(&existing_branch_ref, fRepository,
				existingBranchName.String(), type);
			// if the selected ref is not a branch it may be a tag, let's try with that
			if (status < 0) {
				BString tag("refs/tags/");
				tag.Append(existingBranchName);
				check(git_reference_lookup(&existing_branch_ref, fRepository,
					tag.String()));
			}


			// Get the commit at the tip of the existing branch
			git_oid commit_id;
			git_reference_name_to_id(&commit_id, fRepository, git_reference_name(existing_branch_ref));
			git_commit_lookup(&commit, fRepository, &commit_id);

			// Create a new branch pointing at this commit
			check(git_branch_create(&new_branch_ref, fRepository, newBranchName.String(), commit, 0));

			// Clean up
			git_commit_free(commit);
			git_reference_free(existing_branch_ref);
			git_reference_free(new_branch_ref);

		} catch (const GitException &e) {
			// Clean up in case of an exception
			if (existing_branch_ref != nullptr) git_reference_free(existing_branch_ref);
			if (new_branch_ref != nullptr) git_reference_free(new_branch_ref);
			if (commit != nullptr) git_commit_free(commit);

			// Re-throw the exception
			throw;
		}
	}

	// TODO: Use a typedef
	std::vector<std::pair<BString, BString>>
	GitRepository::GetFiles()
	{
		git_status_options statusopt;
		git_status_init_options(&statusopt, GIT_STATUS_OPTIONS_VERSION);
		statusopt.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
		statusopt.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;
		git_status_list *status = nullptr;
		check(git_status_list_new(&status, fRepository, &statusopt));

		std::vector<std::pair<BString, BString>> fileStatuses;
		size_t maxi = git_status_list_entrycount(status);
		for (size_t i = 0; i < maxi; ++i) {
			const git_status_entry *s = git_status_byindex(status, i);

			if (s->status == GIT_STATUS_CURRENT)
				continue;

			BString filePath = s->index_to_workdir->old_file.path;
			BString fileStatus;

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
//
			else if (s->status & GIT_STATUS_INDEX_DELETED)
				fileStatus = B_TRANSLATE("File deleted from index");

			fileStatuses.emplace_back(filePath, fileStatus);
		}

		git_status_list_free(status);

		return fileStatuses;
	}

	const BPath&
	GitRepository::Clone(const BString& url, const BPath& localPath,
							git_indexer_progress_cb callback,
							git_credential_acquire_cb authentication_callback)
	{
		git_repository *repo = nullptr;

		try {
			git_libgit2_init();
			git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
			git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
			callbacks.transfer_progress = callback;
			clone_opts.fetch_opts.callbacks = callbacks;
			clone_opts.fetch_opts.callbacks.credentials = authentication_callback;

			check(git_clone(&repo, url.String(), localPath.Path(), &clone_opts));
			git_repository_free(repo);
			return localPath;
		} catch (const GitException &e) {
			if (repo != nullptr)
				git_repository_free(repo);
			throw;
		}

	}

	void
	GitRepository::Fetch(bool prune)
	{
		git_remote *remote = nullptr;
		git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
		fetch_opts.callbacks.credentials = GitCredentialsWindow::authentication_callback;
		if (prune)
			fetch_opts.prune = GIT_FETCH_PRUNE;
		else
			fetch_opts.prune = GIT_FETCH_NO_PRUNE;

		/* lookup the remote */
		try {
			check(git_remote_lookup(&remote, fRepository, "origin"));
			check(git_remote_fetch(remote, nullptr, &fetch_opts, nullptr));
			git_remote_free(remote);
		} catch (const GitException &e) {
			if (remote != nullptr) git_remote_free(remote);
			throw;
		}
	}
/*
	void
	GitRepository::Merge(BString source, BString dest)
	{
		// TODO
	}

	// DO NOT USE
	// WARNING: The implementation of Pull is flawed and must be reworked but there are very
	// examples around let alone in the libgit2 site
	PullResult
	GitRepository::Pull(BString branchName)
	{
		git_remote *remote = nullptr;
		git_annotated_commit* heads[1];

		auto CleanUp = [this, &heads, &remote]() -> void {
			git_annotated_commit_free(heads[0]);
			if (fRepository != nullptr) git_repository_state_cleanup(fRepository);
			if (remote != nullptr) git_remote_free(remote);
		};

		try {

			// Fetch the changes from the remote
			Fetch();

			auto fetchhead_ref_cb = [](const char* name, const char* url, const git_oid* oid,
				unsigned int is_merge, void* payload_v) -> int
			{
				struct fetch_payload* payload = (struct fetch_payload*) payload_v;
				if (is_merge) {
					strcpy(payload->branch, name);
					memcpy(&payload->branch_oid, oid, sizeof(git_oid));
					LogInfo(name);
				}
				return 0;
			};

			struct fetch_payload payload;
			git_repository_fetchhead_foreach(fRepository, fetchhead_ref_cb, &payload);
			check(git_annotated_commit_lookup(&heads[0], fRepository, &payload.branch_oid));

			// Perform a merge operation
			git_merge_analysis_t merge_analysis_t;
			git_merge_preference_t merge_preference_t;
			check(git_merge_analysis(&merge_analysis_t, &merge_preference_t,
									 fRepository, (const git_annotated_commit**)&heads[0], 1));

			if (merge_analysis_t & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
				CleanUp();
				return PullResult::UpToDate;
			} else if (merge_analysis_t & GIT_MERGE_ANALYSIS_FASTFORWARD) {
				check(_FastForward(&payload.branch_oid, (merge_analysis_t & GIT_MERGE_ANALYSIS_UNBORN)));
				CleanUp();
				return PullResult::FastForwarded;
			} else if (merge_analysis_t & GIT_MERGE_ANALYSIS_NORMAL) {
				git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
				git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

				merge_opts.file_flags = GIT_MERGE_FILE_STYLE_DIFF3;

				checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE | GIT_CHECKOUT_ALLOW_CONFLICTS;

				check(git_merge(fRepository,
								(const git_annotated_commit **)&heads[0], 1,
								&merge_opts, &checkout_opts));
			}

			// If we get here, we actually performed the merge above
			git_index* index;
			check(git_repository_index(&index, fRepository));
			if (!git_index_has_conflicts(index))
				check(_CreateCommit(index, "Merge"));
			else
				ReportConflicts(index);
			git_index_free(index);
			CleanUp();
			return PullResult::Merged;
		} catch (const GitException &e) {
			CleanUp();
			throw;
		}
	}

	// DO NOT USE
	// WARNING: The implementation of Pull rebase is flawed and must be reworked but there are very
	// examples around let alone in the libgit2 site
	void
	GitRepository::PullRebase()
	{
		Fetch();

		git_rebase *rebase = nullptr;
		git_rebase_options rebase_opts = GIT_REBASE_OPTIONS_INIT;
		git_oid upstream_oid, branch_oid;
		git_annotated_commit *upstream_head = nullptr, *branch_head = nullptr;

		// Get OIDs for the branch and its upstream
		BString current_branch = GetCurrentBranch();
		BString local_ref, remote_ref;
		local_ref.SetToFormat("refs/heads/%s", current_branch.String());
		remote_ref.SetToFormat("refs/remotes/origin/%s", current_branch.String());

		git_reference_name_to_id(&branch_oid, fRepository, local_ref.String());
		git_reference_name_to_id(&upstream_oid, fRepository, remote_ref.String());

		git_annotated_commit_lookup(&branch_head, fRepository, &branch_oid);
		git_annotated_commit_lookup(&upstream_head, fRepository, &upstream_oid);

		// Initialize rebase
		git_rebase_init(&rebase, fRepository, branch_head, upstream_head, NULL, &rebase_opts);

		// Perform the rebase operations
		auto signature = _GetSignature();
		git_rebase_operation *operation;
		while (git_rebase_next(&operation, rebase) == 0) {
			git_oid *id = nullptr;
			auto result = git_rebase_commit(id, rebase, nullptr, signature, nullptr, nullptr);
			LogInfo("commit: %s result: %d", id, result);
		}

		check(git_rebase_finish(rebase, nullptr));
		git_signature_free(signature);
		git_rebase_free(rebase);

	}

	void
	GitRepository::Push()
	{
	}*/

	BString
	GitRepository::_ConfigGet(git_config *cfg, const char *key)
	{
	  git_config_entry *entry = nullptr;
	  BString ret;
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
	GitRepository::StashSave(BString message)
	{
		git_oid saved_stash;
		git_signature *signature = nullptr;
		try {
			signature = _GetSignature();

			// TODO: consider using GIT_STASH_INCLUDE_UNTRACKED
			check(git_stash_save(&saved_stash, fRepository, signature, message.String(), 0));

			git_signature_free(signature);
		} catch (const GitException &e) {
			if (signature != nullptr)
				git_signature_free(signature);
			throw;
		}

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


	std::vector<BString>
	GitRepository::GetTags()
	{
		std::vector<BString> tags;

		auto lambda = [](git_reference *ref, void *payload) -> int
		{
			auto tags = reinterpret_cast<std::vector<BString>*>(payload);
			BString ref_name(git_reference_name(ref));
			if (!ref_name.IFindFirst("refs/tags/")) {
				ref_name.RemoveAll("refs/tags/");
				tags->push_back(ref_name);
				// return 1;
			}
			return 0;
		};
		check(git_reference_foreach(fRepository, lambda, &tags));

		return tags;
	}

	git_signature *
	GitRepository::_GetSignature()
	{
		git_signature *signature = nullptr;

		// TODO - Put this into Genio Prefs
		git_config* cfg = nullptr;
		git_config* cfg_snapshot = nullptr;

		git_config_open_ondisk(&cfg, "/boot/home/config/settings/git/config");
		git_config_snapshot(&cfg_snapshot, cfg);

		const char *user_name, *user_email;
		check(git_config_get_string(&user_name, cfg_snapshot, "user.name"));
		check(git_config_get_string(&user_email, cfg_snapshot, "user.email"));
		check(git_signature_now(&signature, user_name, user_email));

		return signature;
	}

	int
	GitRepository::_FastForward(const git_oid *target_oid, int is_unborn)
	{
		git_checkout_options ff_checkout_options = GIT_CHECKOUT_OPTIONS_INIT;
		git_reference *target_ref = nullptr;
		git_reference *new_target_ref = nullptr;
		git_object *target = nullptr;
		int err = 0;

		try {
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
			std::vector<BString> files;
			ff_checkout_options.notify_flags =	GIT_CHECKOUT_NOTIFY_CONFLICT;
			ff_checkout_options.notify_cb = checkout_notify;
			ff_checkout_options.notify_payload = &files;
			ff_checkout_options.checkout_strategy = GIT_CHECKOUT_SAFE;
			int status = git_checkout_tree(fRepository, target, &ff_checkout_options);
			if (status < 0) {
				throw GitConflictException(status, git_error_last()->message, files);
			}

			// Move the target reference to the target OID
			check(git_reference_set_target(&new_target_ref, target_ref, target_oid,	NULL));

			git_reference_free(target_ref);
			git_reference_free(new_target_ref);
			git_object_free(target);

			return err;
		} catch (const GitException &e) {
			if (target_ref != nullptr) git_reference_free(target_ref);
			if (new_target_ref != nullptr) git_reference_free(new_target_ref);
			if (target != nullptr) git_object_free(target);
			throw;
		}

	}

	int
	GitRepository::_CreateCommit(git_index* index, const char* message)
	{
		git_tree* tree = nullptr;
		git_commit* parent = nullptr;
		git_oid tree_id;
		git_oid commit_id;
		git_oid	parent_id;
		git_signature* signature = _GetSignature();

		// Init tree_id
		int ret = git_index_write_tree(&tree_id, index);
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
			ret = git_commit_create_v(&commit_id, fRepository, "HEAD", signature, signature, NULL,
					message, tree, 0);
			return ret;
		}

		ret = git_commit_lookup(&parent, fRepository, &parent_id);
		if (ret < 0)
			return ret;

		// Create commit
		ret = git_commit_create_v(&commit_id, fRepository, "HEAD", signature, signature, NULL,
				message, tree, 1, parent);

		if (tree != nullptr)
			git_tree_free(tree);
		if (parent != nullptr)
			git_commit_free(parent);
		if (signature != nullptr)
			git_signature_free(signature);

		return ret;
	}

	void
	GitRepository::_CreateInitialCommit()
	{
		git_signature *sig = nullptr;
		git_index *index = nullptr;
		git_oid tree_id;
		git_oid commit_id;
		git_tree *tree = nullptr;

		try {
			sig = _GetSignature();
			/* Now let's create an empty tree for this commit */
			check(git_repository_index(&index, fRepository));
			check(git_index_write_tree(&tree_id, index));
			git_index_free(index);

			check(git_tree_lookup(&tree, fRepository, &tree_id));

			check(git_commit_create_v(&commit_id, fRepository, "HEAD", sig, sig,
				NULL, "Initial commit", tree, 0));

			git_tree_free(tree);
			git_signature_free(sig);
		} catch (const GitException &e) {
			if (sig != nullptr)
				git_signature_free(sig);
			if (tree != nullptr)
				git_tree_free(tree);
			throw;
		}
	}
}
