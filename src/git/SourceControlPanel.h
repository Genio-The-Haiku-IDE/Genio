/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once


#include <LayoutBuilder.h>
#include <MenuField.h>
#include <ObjectList.h>
#include <OptionPopUp.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <View.h>


#include "OptionList.h"
#include "ToolBar.h"



enum Messages {
	MsgChangeProject,
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
	MsgStashApply,
	MsgSwitchBranch,
	MsgRenameBranch,
	MsgDeleteBranch,
	MsgNewBranch,
	MsgNewTag
};

class ProjectFolder;
class RepositoryView;
class SourceControlPanel : public BView {
public:
							SourceControlPanel();
							~SourceControlPanel();

	void					MessageReceived(BMessage *message);
	void					AttachedToWindow();
	void					DetachedFromWindow();

private:

	Genio::UI::OptionList<ProjectFolder *>* fProjectMenu;
	Genio::UI::OptionList<BString>*	fBranchMenu;
	ToolBar*				fToolBar;
	BCardLayout*			fMainLayout;
	RepositoryView*			fRepositoryView;
	BScrollView*			fRepositoryViewScroll;
	BView*					fChangesView;
	BView*					fLogView;
	BObjectList<ProjectFolder>* fProjectList;
	ProjectFolder*			fActiveProject;
	ProjectFolder*			fSelectedProject;
	BString					fCurrentBranch;

	void					_UpdateProjectList();
	void					_UpdateBranchList(bool invokeItemMessage = true);
	void					_InitToolBar();
	void					_InitRepositoryView();
	void					_UpdateRepositoryView();
	void					_InitChangesView();
	void					_InitLogView();
	void					_ShowOptionsMenu(BPoint where);
	void					_ShowGitNotification(const BString& text);
};