/*
 * Copyright 2025, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <Looper.h>

#include "GitRepository.h"

#if 0
namespace Genio::Git {
	class GitRepository;
}
#endif

using Genio::Git::GitRepository;

class ProjectFolder;
class SourceControlManager : public BLooper {
public:
	SourceControlManager(const char* fullPath);
	
	void MessageReceived(BMessage* message) override;

	void Init(bool createInitalCommit = true);
	bool IsInitialized();
	
	std::vector<BString> GetTags(size_t maxTags = Genio::Git::MAX_ELEMENTS) const;

	std::vector<BString> GetBranches(git_branch_t type = GIT_BRANCH_LOCAL,
									size_t maxBranches = Genio::Git::MAX_ELEMENTS) const;
	
	void SwitchBranch(const char* branch);
	BString GetCurrentBranch() const;
	void DeleteBranch(const BString branch,
					git_branch_t type);
	void RenameBranch(const BString oldName,
					const BString newName,
					git_branch_t type);
	void CreateBranch(const BString existingBranchName,
					git_branch_t type,
					const BString newBranchName);

	void Fetch(bool prune = false);
	void Merge(const BString source, const BString dest);
	//PullResult Pull(const BString branchName);
	void PullRebase();
	void Push();
	void StashSave(const BString message);
	void StashPop();
	void StashApply();

private:
	void _BranchChanged(const char* branch);

	ProjectFolder* fProject;
	GitRepository* fGitRepository;
};
