/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <Catalog.h>
#include <OutlineListView.h>

#include "GitRepository.h"
#include "GMessage.h"
#include "Log.h"
#include "SourceControlPanel.h"
#include "ProjectFolder.h"
#include "Utils.h"

enum RepositoryViewMessages {
	kUndefinedMessage,
	kInvocationMessage
};

enum ItemType {
	kLocalBranch,
	kRemoteBranch,
	kTag
};

class BranchItem;
class RepositoryView : public BOutlineListView {
public:

					 RepositoryView();
	virtual 		~RepositoryView();

	void			MouseDown(BPoint where);
	void			MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	void			AttachedToWindow();
	void			DetachedFromWindow();
	void			MessageReceived(BMessage* message);
	void			SelectionChanged();

	void			UpdateRepository(ProjectFolder *selectedProject, const BString &currentBranch);

	auto			GetSelectedItem();

private:

	void			_ShowPopupMenu(BPoint where);

	BranchItem*		_InitEmptySuperItem(const BString &label);

	BString			fRepositoryPath;
	ProjectFolder*	fSelectedProject;
	BString			fCurrentBranch;
};