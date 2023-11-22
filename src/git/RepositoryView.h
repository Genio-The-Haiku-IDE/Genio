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
#include "StyledItem.h"
#include "ProjectFolder.h"
#include "Utils.h"

enum RepositoryViewMessages {
	kUndefinedMessage
};

enum ItemType {
	kLocalBranch,
	kRemoteBranch,
	kTag
};

class RepositoryView : public BOutlineListView {
public:

					 RepositoryView();
					 RepositoryView(BString &repository_path);
					 RepositoryView(const char *repository_path);
	virtual 		~RepositoryView();

	void			MouseDown(BPoint where);
	void			MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	void			AttachedToWindow();
	void			DetachedFromWindow();
	void			MessageReceived(BMessage* message);
	void			SelectionChanged();

private:

	void			_UpdateRepository();
	void			_ShowPopupMenu(BPoint where);

	BString			fRepositoryPath;
	ProjectFolder*	fSelectedProject;
	BString			fCurrentBranch;
};