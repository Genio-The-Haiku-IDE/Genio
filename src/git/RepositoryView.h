/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <OutlineListView.h>


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
class RepositoryView : public BOutlineListView {
public:

					 RepositoryView();
	virtual 		~RepositoryView();

	virtual void	MouseUp(BPoint where);
	virtual void	MouseDown(BPoint where);
	virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	MessageReceived(BMessage* message);
	virtual void	SelectionChanged();

	void			UpdateRepository(const ProjectFolder *project, const BString &currentBranch);
private:
	void			_UpdateRepositoryTask(const Genio::Git::GitRepository* repo, const BString& branch);
	
	BranchItem*		_InitEmptySuperItem(const BString &label);
	void			_BuildBranchTree(const BString &branch, uint32 branchType, const auto& checker);

	void			_ShowPopupMenu(BPoint where);

	BString			fCurrentBranch;
};