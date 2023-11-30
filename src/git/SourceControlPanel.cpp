/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
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
#include <PathMonitor.h>
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

#include "GitAlert.h"
#include "GTextAlert.h"

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
	if (gMainWindow->Lock()) {
		gMainWindow->StartWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		gMainWindow->StartWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);

		auto projectBrowser = gMainWindow->GetProjectBrowser();
		if (projectBrowser != nullptr)
			projectBrowser->StartWatching(this, B_PATH_MONITOR);

		gMainWindow->Unlock();
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
	if (gMainWindow->Lock()) {
		gMainWindow->StopWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		gMainWindow->StopWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);

		auto projectBrowser = gMainWindow->GetProjectBrowser();
		if (projectBrowser != nullptr)
			projectBrowser->StopWatching(this, B_PATH_MONITOR);

		gMainWindow->Unlock();
	}
}


void
SourceControlPanel::MessageReceived(BMessage *message)
{
	try {
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
					case B_PATH_MONITOR:
					{
						LogInfo("B_PATH_MONITOR");
						fCurrentBranch = fSelectedProject->GetRepository()->GetCurrentBranch();
						_UpdateBranchList(false);
						GMessage *message = new GMessage({{"what", MsgSwitchBranch}, {"value", fCurrentBranch}});
						SendNotices(MsgSwitchBranch, message);
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
				break;
			}
			case MsgShowChangesPanel: {
				LogInfo("MsgShowChangesPanel");
				if (fMainLayout->VisibleIndex() != kViewIndexChanges)
					fMainLayout->SetVisibleItem(kViewIndexChanges);
				fToolBar->ToggleActionPressed(MsgShowChangesPanel);
				break;
			}
			case MsgShowLogPanel: {
				LogInfo("MsgShowLogPanel");
				if (fMainLayout->VisibleIndex() != kViewIndexLog)
					fMainLayout->SetVisibleItem(kViewIndexLog);
				fToolBar->ToggleActionPressed(MsgShowLogPanel);
				break;
			}
			case MsgShowOptionsMenu: {
				auto button = fToolBar->FindButton(MsgShowOptionsMenu);
				auto where = button->Frame().LeftBottom();
				LogInfo("MsgShowOptionsMenu: p(%f, %f)", where.x, where.y);
				_ShowOptionsMenu(where);
				break;
			}
			case MsgFetch: {
				LogInfo("MsgFetch");
				Genio::Git::GitRepository git(fSelectedProject->Path().String());
				git.Fetch();
				_ShowGitNotification(B_TRANSLATE("Fetch completed."));
				_UpdateBranchList();
				break;
			}
			case MsgFetchPrune: {
				LogInfo("MsgFetchPrune");
				Genio::Git::GitRepository git(fSelectedProject->Path().String());
				git.Fetch(true);
				_ShowGitNotification(B_TRANSLATE("Fetch prune completed."));
				_UpdateBranchList();
				break;
			}
			case MsgStashSave: {
				LogInfo("MsgStashSave");

				BString stash_message;
				stash_message << "WIP on " << fCurrentBranch << B_UTF8_ELLIPSIS;
				auto alert = new GTextAlert("Stash", B_TRANSLATE("Enter a message for this stash"),
					stash_message);
				auto result = alert->Go();
				if (result.Button == GAlertButtons::OkButton) {
					stash_message = result.Result;
				}

				fSelectedProject->GetRepository()->StashSave(stash_message);
				_ShowGitNotification(B_TRANSLATE("Changes stashed."));
				break;
			}
			case MsgStashPop: {
				LogInfo("MsgStashPop");
				Genio::Git::GitRepository git(fSelectedProject->Path().String());
				git.StashPop();
				_ShowGitNotification(B_TRANSLATE("Stashed changes popped."));
				break;
			}
			case MsgStashApply: {
				LogInfo("MsgStashApply");
				Genio::Git::GitRepository git(fSelectedProject->Path().String());
				git.StashApply();
				_ShowGitNotification(B_TRANSLATE("Stashed changes applied."));
				break;
			}
			case MsgChangeProject: {
				fSelectedProject = (ProjectFolder *)message->GetPointer("value");
				LogInfo("MsgSelectProject: to %s", fSelectedProject->Name().String());
				_UpdateBranchList();
				SendNotices(message->what, message);
				break;
			}
			case MsgSwitchBranch: {
				auto branch = (BString)message->GetString("value");
				auto repo = fSelectedProject->GetRepository();
				repo->SwitchBranch(branch);
				fCurrentBranch = repo->GetCurrentBranch();
				message->SetString("value", fCurrentBranch);
				// we don't want to invoke the message attached to the selected menu item
				// when the list is updated from here
				// we also don't need to update the OptionList itself
				auto source = BString(message->GetString("source_item", ""));
				if (source == "popup_menu")
					_UpdateBranchList(false);
				SendNotices(message->what, message);
				LogInfo("MsgSwitchBranch: %s", fCurrentBranch.String());
				break;
			}
			case MsgRenameBranch: {
				auto selected_branch = BString(message->GetString("value"));
				git_branch_t branch_type = static_cast<git_branch_t>(message->GetInt32("type",-1));
				auto alert = new GTextAlert(B_TRANSLATE("Rename branch"),
					B_TRANSLATE("Rename branch:"), selected_branch);
				auto result = alert->Go();
				if (result.Button == GAlertButtons::OkButton) {
					auto repo = fSelectedProject->GetRepository();
					repo->RenameBranch(selected_branch, result.Result, branch_type);
					_ShowGitNotification(B_TRANSLATE("Branch renamed succesfully."));
					_UpdateBranchList();
					LogInfo("MsgRenameBranch: %s renamed to %s", selected_branch.String(),
						result.Result.String());
				}
				break;
			}
			case MsgDeleteBranch: {
				auto selected_branch = BString(message->GetString("value"));
				StringFormatter fmt;
				fmt.Substitutions["%branch%"] = selected_branch;

				BString text(fmt.Replace(B_TRANSLATE("Deleting branch: \"%branch%\".\n\n"
					"After deletion, the item will be lost.\n"
					"Do you really want to delete it?")));
				BAlert* alert = new BAlert(B_TRANSLATE("Delete dialog"),
					text.String(),
					B_TRANSLATE("Cancel"), B_TRANSLATE("Delete"), nullptr,
					B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
				alert->SetShortcut(0, B_ESCAPE);
				int32 choice = alert->Go();
				if (choice == 1) {
					git_branch_t branch_type = static_cast<git_branch_t>(message->GetInt32("type",-1));
					auto repo = fSelectedProject->GetRepository();
					repo->DeleteBranch(selected_branch, branch_type);
					_ShowGitNotification(B_TRANSLATE("Branch deleted succesfully."));
					_UpdateBranchList();
					LogInfo("MsgDeleteBranch: %s", selected_branch.String());
				}
				break;
			}
			case MsgNewBranch: {
				auto selected_branch = BString(message->GetString("value"));
				BString new_local_name(selected_branch);
				new_local_name.RemoveAll("origin/");
				git_branch_t branch_type = static_cast<git_branch_t>(message->GetInt32("type",-1));
				auto alert = new GTextAlert(B_TRANSLATE("Rename branch"),
					B_TRANSLATE("Rename branch:"), selected_branch);
				auto result = alert->Go();
				if (result.Button == GAlertButtons::OkButton) {
					auto repo = fSelectedProject->GetRepository();
					repo->CreateBranch(selected_branch, branch_type, result.Result);
					_ShowGitNotification(B_TRANSLATE("Branch created succesfully."));
					_UpdateBranchList();
					LogInfo("MsgRenameBranch: %s created from %s", selected_branch.String(),
						result.Result.String());
				}
				break;
			}
			// Pull must be fixed and is disabled
			// case MsgPull: {
				// auto selected_branch = BString(message->GetString("selected_branch"));
				// auto result = fSelectedProject->GetRepository()->Pull(selected_branch);
				// switch (result) {
					// case PullResult::UpToDate: {
						// _ShowGitNotification(B_TRANSLATE("The branch is up to date."));
						// break;
					// }
					// case PullResult::FastForwarded: {
						// _ShowGitNotification(B_TRANSLATE("The branch has been fast-forwarded."));
						// break;
					// }
					// case PullResult::Merged: {
						// _ShowGitNotification(B_TRANSLATE("The branch has been merged."));
						// break;
					// }
				// }
				// _ShowGitNotification(B_TRANSLATE("Pull completed."));
				// LogInfo("MsgPull: %s", selected_branch.String());
				// break;
			// }
			// Pull rebase is seriously flawed. We disable this until we get it work properly
			// case MsgPullRebase: {
				// auto selected_branch = BString(message->GetString("selected_branch"));
				// fSelectedProject->GetRepository()->PullRebase();
				// _ShowGitNotification(B_TRANSLATE("Pull rebase completed."));
				// LogInfo("MsgPullRebase: %s", selected_branch.String());
				// break;
			// }
			default:
			break;
		}
	} catch(GitConflictException &ex) {
		auto alert = new GitAlert(B_TRANSLATE("Conflicts"),
			B_TRANSLATE(ex.Message().String()), ex.GetFiles());
		alert->Go();
		_UpdateBranchList(false);
	} catch(GitException &ex) {
		OKAlert("SourceControlPanel", ex.Message().String(), B_STOP_ALERT);
	}	catch(std::exception &ex) {
		OKAlert("SourceControlPanel", ex.what(), B_STOP_ALERT);
	} catch(...) {
		OKAlert("SourceControlPanel", B_TRANSLATE("An unknown error occurred."), B_STOP_ALERT);
	}
}


void
SourceControlPanel::_UpdateProjectList()
{
	if ((fProjectList != nullptr)) {
		fProjectMenu->SetTarget(this);
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
			true,
			[&selected = this->fSelectedProject](auto item)
			{
				return (item == selected);
			}
		);
		_UpdateBranchList();
	}
}


void
SourceControlPanel::_UpdateBranchList(bool invokeItemMessage)
{
	if (fSelectedProject != nullptr) {
		Genio::Git::GitRepository repo(fSelectedProject->Path().String());
		auto branches = repo.GetBranches();
		fCurrentBranch = repo.GetCurrentBranch();
		LogInfo("current branch is set to %s", fCurrentBranch.String());
		fBranchMenu->SetTarget(this);
		fBranchMenu->MakeEmpty();
		fBranchMenu->AddIterator(branches,
			MsgSwitchBranch,
			[](auto &item) { return item; },
			invokeItemMessage,
			[&current_branch=this->fCurrentBranch](auto &item) { return (item == current_branch);}
		);
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
SourceControlPanel::_ShowOptionsMenu(BPoint where)
{
	StringFormatter fmt;
	fmt.Substitutions["%current_branch%"] = fCurrentBranch;

	auto optionsMenu = new BPopUpMenu("Options", false, false);
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Fetch"), new BMessage(MsgFetch)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Fetch prune"),	new BMessage(MsgFetchPrune)));
	optionsMenu->AddSeparatorItem();

	// Pull and pull rebase are seriously flawed. We disable this until we get it work properly
	// Push and Merge are not implemented
	// optionsMenu->AddItem(
		// new BMenuItem(
			// fmt << B_TRANSLATE("Pull \"origin/%current_branch%\""),
			// new BMessage(MsgPull)));
	// optionsMenu->AddItem(
		// new BMenuItem(
			// fmt << B_TRANSLATE("Pull rebase \"origin/%current_branch%\""),
			// new BMessage(MsgPullRebase)));
	// optionsMenu->AddItem(
		// new BMenuItem(
			// fmt << B_TRANSLATE("Push \"%current_branch%\" to \"origin\\%current_branch%\""),
			// new BMessage(MsgPush)));
	// optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Merge"), new BMessage(MsgMerge)));
	// optionsMenu->AddSeparatorItem();

	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Stash changes"), new BMessage(MsgStashSave)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Stash pop changes"), new BMessage(MsgStashPop)));
	optionsMenu->AddItem(new BMenuItem(B_TRANSLATE("Stash apply changes"), new BMessage(MsgStashApply)));
	optionsMenu->SetTargetForItems(this);
	optionsMenu->Go(fToolBar->ConvertToScreen(where), true);
	delete optionsMenu;
}

void
SourceControlPanel::_ShowGitNotification(const BString &text)
{
	ShowNotification("Genio", fSelectedProject->Path().String(),
		fSelectedProject->Path().String(),
		text);
}