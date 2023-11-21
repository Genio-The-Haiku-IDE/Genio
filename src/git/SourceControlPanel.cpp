/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "SourceControlPanel.h"

#include <Autolock.h>
#include <Button.h>
#include <ObjectList.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <ObjectList.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <StringItem.h>
#include <SeparatorView.h>
#include <ScrollView.h>
#include <StringView.h>

#include "GitRepository.h"
#include "GenioWindow.h"
#include "GenioWindowMessages.h"
#include "Log.h"
#include "StringFormatter.h"
#include "StyledItem.h"
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SourceControlPanel"

enum ViewIndex {
	kViewIndexRepository = 0,
	kViewIndexChanges = 1,
	kViewIndexLog = 2
};

SourceControlPanel::SourceControlPanel()
	: BView(B_TRANSLATE("Source Control"), B_WILL_DRAW | B_FRAME_EVENTS ),
	fProjectMenu(nullptr),
	fBranchMenu(nullptr),
	fProjectList(nullptr),
	fActiveProject(nullptr),
	fSelectedProject(nullptr),
	fCurrentBranch(nullptr)
{

	fProjectList = gMainWindow->GetProjectList();
	fActiveProject = gMainWindow->GetActiveProject();

	fProjectMenu = new OptionList<ProjectFolder *>("ProjectMenu",
		B_TRANSLATE("Project:"),
		B_TRANSLATE("Choose project" B_UTF8_ELLIPSIS));
	fBranchMenu = new OptionList<BString>("BranchMenu",
		B_TRANSLATE("Current branch:"),
		B_TRANSLATE("Choose branch" B_UTF8_ELLIPSIS));

	_InitToolBar();
	_InitRepositoryView();
	_UpdateRepositoryView();
	_InitChangesView();
	_InitLogView();

	fMainLayout = BLayoutBuilder::Cards<>()
		.Add(fRepositoryViewScroll)
		.Add(fChangesView)
		.Add(fLogView)
		.SetVisibleItem((int32)kViewIndexRepository);

	fToolBar->ToggleActionPressed(MsgShowRepositoryPanel);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_BORDER_INSETS)
		.SetInsets(0)
		.AddGroup(B_VERTICAL)
			.SetInsets(5)
			.Add(fProjectMenu)
			.Add(fBranchMenu)
			.End()
		.Add(fToolBar)
		.Add(fMainLayout)
		.End();
}


SourceControlPanel::~SourceControlPanel()
{
	delete fLogView;
	delete fChangesView;
	delete fRepositoryView;
	delete fToolBar;
	delete fBranchMenu;
	delete fProjectMenu;
}


void
SourceControlPanel::AttachedToWindow()
{
	auto window = this->Window();
	if (window->Lock()) {
		window->StartWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		window->StartWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);
		window->Unlock();
	}

	_UpdateProjectList();

	fProjectMenu->SetTarget(this);
	fBranchMenu->SetTarget(this);
	fToolBar->SetTarget(this);
	fRepositoryView->SetTarget(this);
}


void
SourceControlPanel::DetachedFromWindow()
{
	auto window = this->Window();
	if (window->Lock()) {
		window->StopWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		window->StopWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);
		window->Unlock();
	}
}


void
SourceControlPanel::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE: {
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case MSG_NOTIFY_PROJECT_LIST_CHANGED:
				{
					LogInfo("MSG_NOTIFY_PROJECT_LIST_CHANGED");
					fProjectList = gMainWindow->GetProjectList();
					_UpdateProjectList();
					break;
				}
				case MSG_NOTIFY_PROJECT_SET_ACTIVE:
				{
					LogInfo("MSG_NOTIFY_PROJECT_SET_ACTIVE");
					fActiveProject = gMainWindow->GetActiveProject();
					fSelectedProject = fActiveProject;
					_UpdateProjectList();
					break;
				}
				default:
					break;
			}
			break;
		}
		case MsgShowRepositoryPanel: {
			LogInfo("MsgShowRepositoryPanel");
			if (fMainLayout->VisibleIndex() != kViewIndexRepository)
				fMainLayout->SetVisibleItem(kViewIndexRepository);
			fToolBar->ToggleActionPressed(MsgShowRepositoryPanel);
		}
		break;
		case MsgShowChangesPanel: {
			LogInfo("MsgShowChangesPanel");
			if (fMainLayout->VisibleIndex() != kViewIndexChanges)
				fMainLayout->SetVisibleItem(kViewIndexChanges);
			fToolBar->ToggleActionPressed(MsgShowChangesPanel);
		}
		break;
		case MsgShowLogPanel: {
			LogInfo("MsgShowLogPanel");
			if (fMainLayout->VisibleIndex() != kViewIndexLog)
				fMainLayout->SetVisibleItem(kViewIndexLog);
			fToolBar->ToggleActionPressed(MsgShowLogPanel);
		}
		break;
		case MsgShowOptionsMenu: {
			auto button = fToolBar->FindButton(MsgShowOptionsMenu);
			auto where = button->Frame().LeftBottom();
			LogInfo("MsgShowOptionsMenu: p(%f, %f)", where.x, where.y);
			_ShowOptionsMenu(where);
		}
		break;
		case MsgFetch: {
			LogInfo("MsgFetch");
			Genio::Git::GitRepository git(fSelectedProject->Path().String());
			git.Fetch();
		}
		break;
		case MsgFetchPrune: {
			LogInfo("MsgFetchPrune");
			Genio::Git::GitRepository git(fSelectedProject->Path().String());
			git.Fetch(true);
		}
		break;
		case MsgStashSave: {
			LogInfo("MsgStashSave");
			Genio::Git::GitRepository git(fSelectedProject->Path().String());
			git.StashSave();
		}
		break;
		case MsgStashPop: {
			LogInfo("MsgStashPop");
			Genio::Git::GitRepository git(fSelectedProject->Path().String());
			git.StashPop();
		}
		case MsgStashApply: {
			LogInfo("MsgStashApply");
			Genio::Git::GitRepository git(fSelectedProject->Path().String());
			git.StashApply();
		}
		break;
		case MsgChangeProject : {
			fSelectedProject = (ProjectFolder *)message->GetPointer("value");
			LogInfo("MsgSelectProject: %s", fSelectedProject->Name().String());
			auto path = fSelectedProject->Path().String();
			LogInfo("MsgSelectProject: %s", path);
			_UpdateBranchList();
			_UpdateRepositoryView();

			SendNotices(message->what, message);
		}
		break;
		case MsgSwitchBranch : {
			fCurrentBranch = (BString)message->GetString("value");
			SendNotices(message->what, message);
			LogInfo("**MsgSwitchBranch: %s", fCurrentBranch.String());
		}
		break;
		case MsgPull : {
			auto selected_branch = message->GetString("selected_branch");
			auto current_branch = message->GetString("current_branch");
			LogInfo("MsgPull: %s into %s", selected_branch, current_branch);
		}
		break;
		default:
		break;
	}
}


void
SourceControlPanel::_UpdateProjectList()
{
	if ((fProjectList != nullptr)) {
		fProjectMenu->MakeEmpty();
		fProjectMenu->AddList(fProjectList,
			MsgChangeProject,
			[&active = this->fActiveProject](auto item)
			{
				auto project_name = item->Name();
				if ((active != nullptr) &&
					(active->Name() == project_name))
					project_name.Append("*");
				return project_name;
			},
			[&selected = this->fSelectedProject](auto item)
			{
				return (item == selected);
			}
		);
		fProjectMenu->SetTarget(this);
		_UpdateBranchList();
	}
}


void
SourceControlPanel::_UpdateBranchList()
{
	if (fSelectedProject != nullptr) {
		Genio::Git::GitRepository repo(fSelectedProject->Path().String());
		auto branches = repo.GetBranches();
		fCurrentBranch = repo.GetCurrentBranch();
		fBranchMenu->MakeEmpty();
		fBranchMenu->AddIterator(branches,
			MsgSwitchBranch,
			[](auto &item) { return item; },
			[&current_branch=this->fCurrentBranch](auto &item) { return (item == current_branch);}
		);
		fBranchMenu->SetTarget(this);
	}
}

void
SourceControlPanel::_InitToolBar()
{
	fToolBar = new ToolBar();
	fToolBar->ChangeIconSize(16);
	fToolBar->AddAction(MsgShowRepositoryPanel, B_TRANSLATE("Repository"), "kIconGitRepo");
	fToolBar->AddAction(MsgShowChangesPanel, B_TRANSLATE("Changes"), "kIconGitChanges");
	fToolBar->AddAction(MsgShowLogPanel, B_TRANSLATE("Log"), "kIconGitLog");
	fToolBar->AddGlue();
	fToolBar->AddAction(MsgShowOptionsMenu, B_TRANSLATE("Options"), "kIconGitMore");
}


void
SourceControlPanel::_InitRepositoryView()
{
	fRepositoryView = new RepositoryView();
	fRepositoryViewScroll = new BScrollView(B_TRANSLATE("Repository scroll view"),
		fRepositoryView, B_FRAME_EVENTS | B_WILL_DRAW, true, true, border_style::B_NO_BORDER);
}


void
SourceControlPanel::_InitChangesView()
{
	fChangesView = BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_BORDER_INSETS)
		.AddGroup(B_VERTICAL)
			.AddGlue()
			.Add(new BButton("changes"))
			.AddGlue()
		.End()
		.AddGlue()
		.View();
}


void
SourceControlPanel::_InitLogView()
{
	fLogView = BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_BORDER_INSETS)
		.AddGroup(B_VERTICAL)
			.AddGlue()
			.Add(new BButton("log"))
			.AddGlue()
		.End()
		.AddGlue()
		.View();
}


void
SourceControlPanel::_UpdateRepositoryView()
{
	fRepositoryView->MakeEmpty();

	auto locals = new StyledItem(fRepositoryView, B_TRANSLATE("Local branches"));
	locals->SetPrimaryTextStyle(B_BOLD_FACE);
	fRepositoryView->AddItem(locals);
	// populate local branches
	if (fSelectedProject != nullptr) {
		Genio::Git::GitRepository repo(fSelectedProject->Path().String());
		auto branches = repo.GetBranches(GIT_BRANCH_LOCAL);
		auto current_branch = repo.GetCurrentBranch();
		for(auto &branch : branches) {
			auto item = new StyledItem(fRepositoryView, branch.String(), kLocalBranch);
			fRepositoryView->AddUnder(item, locals);
		}
	}

	auto remotes = new StyledItem(fRepositoryView, B_TRANSLATE("Remotes"));
	remotes->SetPrimaryTextStyle(B_BOLD_FACE);
	fRepositoryView->AddItem(remotes);
	// populate remote branches
	if (fSelectedProject != nullptr) {
		Genio::Git::GitRepository repo(fSelectedProject->Path().String());
		auto branches = repo.GetBranches(GIT_BRANCH_REMOTE);
		auto current_branch = repo.GetCurrentBranch();
		for(auto &branch : branches) {
			auto item = new StyledItem(fRepositoryView, branch.String(), kRemoteBranch);
			fRepositoryView->AddUnder(item, remotes);
		}
	}

	auto tags = new StyledItem(fRepositoryView, B_TRANSLATE("Tags"));
	tags->SetPrimaryTextStyle(B_BOLD_FACE);
	fRepositoryView->AddItem(tags);
	// populate tags
	if (fSelectedProject != nullptr) {
		Genio::Git::GitRepository repo(fSelectedProject->Path().String());
		auto all_tags = repo.GetTags();
		auto current_branch = repo.GetCurrentBranch();
		for(auto &tag : all_tags) {
			auto item = new StyledItem(fRepositoryView, tag.String(), kRemoteBranch);
			fRepositoryView->AddUnder(item, tags);
		}
	}
}


void
SourceControlPanel::_ShowOptionsMenu(BPoint where)
{
	auto optionsMenu = new BPopUpMenu("Options", false, false);
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Fetch"), new BMessage(MsgFetch)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Fetch prune"),	new BMessage(MsgFetchPrune)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Pull"), new BMessage(MsgPull)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Pull rebase"),	new BMessage(MsgPullRebase)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Merge"), new BMessage(MsgMerge)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Push"), new BMessage(MsgPush)));
	optionsMenu->AddSeparatorItem();
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Stash changes"), new BMessage(MsgStashSave)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Stash pop changes"), new BMessage(MsgStashPop)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Stash apply changes"), new BMessage(MsgStashApply)));
	optionsMenu->SetTargetForItems(this);
	optionsMenu->Go(fToolBar->ConvertToScreen(where), true);
	delete optionsMenu;
}