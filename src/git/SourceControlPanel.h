/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once


#include <LayoutBuilder.h>
#include <ObjectList.h>

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
	MsgNewTag,
	MsgInitializeRepository,
	MsgCopyRefName
};


const char* const kSenderProjectOptionList = "ProjectOptionList";
const char* const kSenderBranchOptionList = "BranchOptionList";
const char* const kSenderInitializeRepositoryButton = "InitializeRepositoryButton";
const char* const kSenderRepositoryPopupMenu = "RepositoryPopupMenu";
const char* const kSenderExternalEvent = "ExternalEvent";

class BCheckBox;
class ProjectFolder;
class RepositoryView;
class BScrollView;
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
	BCardLayout*			fPanelsLayout;
	BCardLayout*			fMainLayout;
	RepositoryView*			fRepositoryView;
	BScrollView*			fRepositoryViewScroll;
	BView*					fChangesView;
	BView*					fLogView;
	BView*					fRepositoryNotInitializedView;
	BObjectList<ProjectFolder>* fProjectList;
	BString					fSelectedProjectPath;
	BString					fCurrentBranch;
	BButton*				fInitializeButton;
	BCheckBox*				fDoNotCreateInitialCommitCheckBox;
	BMessageRunner*			fBurstHandler;

	ProjectFolder*			_GetSelectedProject() const;
	void					_UpdateProjectList();
	void					_UpdateBranchList(bool invokeItemMessage = true);

	void					_InitToolBar();
	void					_InitRepositoryView();
	void					_UpdateRepositoryView();
	void					_InitChangesView();
	void					_InitLogView();
	void					_InitRepositoryNotInitializedView();

	void					_ShowOptionsMenu(BPoint where);
	void					_ShowGitNotification(const BString& text);

	void					_ChangeProject(BMessage *message);
	void					_SwitchBranch(BMessage *message);
};
