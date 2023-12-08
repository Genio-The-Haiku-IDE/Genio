/*
 * Copyright 2023, Nexus6 
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

class BranchItem;
class ProjectFolder;
class RepositoryView : public BOutlineListView {
public:

					 RepositoryView();
	virtual 		~RepositoryView();

	virtual void	MouseDown(BPoint where);
	virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	MessageReceived(BMessage* message);
	virtual void	SelectionChanged();

	// TODO: Consider returning BranchItem* directly
	BListItem*		GetSelectedItem();

	void			UpdateRepository(ProjectFolder *selectedProject, const BString &currentBranch);
private:

	BranchItem*		_InitEmptySuperItem(const BString &label);
	void			_BuildBranchTree(const BString &branch, BranchItem *rootItem,
						uint32 branchType, const auto& checker);

	template <typename T>
	T* 				FindItem(const BString& name, T* startItem, bool oneLevelOnly,
						uint32 outlinelevel);

	void			_ShowPopupMenu(BPoint where);

	BString			fRepositoryPath;
	ProjectFolder*	fSelectedProject;
	BString			fCurrentBranch;
};