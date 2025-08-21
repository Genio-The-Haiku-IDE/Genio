/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once


#include <LayoutBuilder.h>
#include <ObjectList.h>

#include "OptionList.h"
#include "ProjectFolder.h"
#include "ToolBar.h"


enum Messages {
	MsgChangeProject,
	MsgShowRepositoryPanel,
	MsgShowChangesPanel,
	MsgShowLogPanel,
	MsgShowActionsMenu,
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

	Genio::UI::OptionList<BString>* fProjectMenu;
	Genio::UI::OptionList<BString>*	fBranchMenu;
	ToolBar*				fToolBar;
	BCardLayout*			fPanelsLayout;
	BCardLayout*			fMainLayout;
	RepositoryView*			fRepositoryView;
	BScrollView*			fRepositoryViewScroll;
	BView*					fChangesView;
	BView*					fLogView;
	BView*					fRepositoryNotInitializedView;
	BString					fCurrentBranch;
	BButton*				fInitializeButton;
	BCheckBox*				fDoNotCreateInitialCommitCheckBox;
	BMessageRunner*			fBurstHandler;

	const ProjectFolder*	_SelectedProject() const;

	void					_UpdateProjectMenu();
	void					_UpdateBranchListMenu(bool invokeItemMessage = true);
	void					_UpdateRepositoryTask(const GitRepository* repo, const BString& branch);
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
	void					_SetCurrentBranch(const ProjectFolder* project, const BString& branch);

	void					_HandleProjectChangedExternalEvent(const ProjectFolder* project);
};
