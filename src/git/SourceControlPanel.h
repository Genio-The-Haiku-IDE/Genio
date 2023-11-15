/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <vector>

#include <LayoutBuilder.h>
#include <MenuField.h>
#include <OptionPopUp.h>
#include <SupportDefs.h>
#include <View.h>

#include "ProjectFolder.h"
#include "../helpers/OptionList.h"
#include "ToolBar.h"

using namespace Genio::UI;

enum Messages {
	MsgChangeProject = '_mcp',
	MsgChangeBranch,
	MsgShowRepositoryPanel,
	MsgShowChangesPanel,
	MsgShowLogPanel,
	MsgShowOptionsMenu,
	MsgFetch,
	MsgFetchPrune,
	MsgPull,
	MsgPullRebase,
	MsgMerge,
	MsgPush,
	MsgStashSave,
	MsgStashPop,
	MsgStashApply
};

class SourceControlPanel : public BView {
public:
							SourceControlPanel();
	void					MessageReceived(BMessage *message);
	void					AttachedToWindow();

private:

	OptionList<ProjectFolder *>* fProjectMenu;
	OptionList<BString>*	fBranchMenu;
	// BMenuField*	fProjectMenu;
	// BMenuField*	fBranchMenu;
	ToolBar*				fToolBar;
	BCardLayout*			fMainLayout;
	BView*					fRepositoryView;
	BScrollView*			fRepositoryViewScroll;
	BView*					fChangesView;
	BView*					fLogView;
	BObjectList<ProjectFolder>* fProjectList;
	ProjectFolder*			fActiveProject;
	ProjectFolder*			fSelectedProject;
	BString					fSelectedBranch;

	void					_UpdateProjectList();
	void					_UpdateBranchList();
	void					_InitToolBar();
	void					_InitRepositoryView();
	void					_InitChangesView();
	void					_InitLogView();
	void					_ShowOptionsMenu(BPoint where);
};