/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include "GOutlineListView.h"


enum RepositoryViewMessages {
	kUndefinedMessage,
	kInvocationMessage
};

enum ItemType {
	kHeader,
	kLocalBranch,
	kRemoteBranch,
	kTag
};

namespace Genio::Git {
	class GitRepository;
}

class BranchItem;
class ProjectFolder;
class RepositoryView : public GOutlineListView {
public:

					 RepositoryView();
	virtual 		~RepositoryView();

			void	MouseMoved(BPoint point, uint32 transit, const BMessage* message) override;
			void	AttachedToWindow() override;
			void	DetachedFromWindow() override;
			void	MessageReceived(BMessage* message) override;
			void	SelectionChanged() override;

	void			UpdateRepository(const ProjectFolder *project, const BString &currentBranch);
private:
	void			ShowPopupMenu(BPoint where) override;

	void			_UpdateRepositoryTask(const Genio::Git::GitRepository* repo, const BString& branch);
	
	BranchItem*		_InitEmptySuperItem(const BString &label);
	void			_BuildBranchTree(const BString &branch, uint32 branchType, const auto& checker);

	// TODO: both RepositoryView and SourceControlPanel keeps track of current branch.
	// Refactor to avoid this if possible
	BString			fCurrentBranch;
};