/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "SourceControlPanel.h"

#include <Alignment.h>
#include <Autolock.h>
#include <Button.h>
#include <Debug.h>
#include <MessageRunner.h>
#include <ObjectList.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Clipboard.h>
#include <LayoutBuilder.h>
#include <ObjectList.h>
#include <OutlineListView.h>
#include <PathMonitor.h>
#include <PopUpMenu.h>
#include <StringItem.h>
#include <SeparatorView.h>
#include <ScrollView.h>
#include <StringView.h>


#include "ConfigManager.h"
#include "GenioApp.h"
#include "GenioWindow.h"
#include "GenioWindowMessages.h"
#include "GitAlert.h"
#include "GitRepository.h"
#include "GTextAlert.h"
#include "Log.h"
#include "ProjectBrowser.h"
#include "ProjectFolder.h"
#include "RepositoryView.h"
#include "StringFormatter.h"
#include "Utils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SourceControlPanel"

using Genio::UI::OptionList;

enum PanelsIndex {
	kPanelsIndexRepository = 0,
	kPanelsIndexChanges = 1,
	kPanelsIndexLog = 2
};

enum MainIndex {
	kMainIndexRepository = 0,
	kMainIndexInitialize = 1,
};

const int kBurstTimeout = 1000000;

SourceControlPanel::SourceControlPanel()
	:
	BView(B_TRANSLATE("Source control"), B_WILL_DRAW | B_FRAME_EVENTS ),
	fProjectMenu(nullptr),
	fBranchMenu(nullptr),
	fCurrentBranch(nullptr),
	fInitializeButton(nullptr),
	fDoNotCreateInitialCommitCheckBox(nullptr),
	fBurstHandler(nullptr)
{
	fProjectMenu = new OptionList<BString>("ProjectMenu",
		B_TRANSLATE("Project:"),
		B_TRANSLATE("Choose project" B_UTF8_ELLIPSIS));
	fBranchMenu = new OptionList<BString>("BranchMenu",
		B_TRANSLATE("Current branch:"),
		B_TRANSLATE("Choose branch" B_UTF8_ELLIPSIS));

	_InitToolBar();
	_InitRepositoryView();
	_InitChangesView();
	_InitLogView();
	_InitRepositoryNotInitializedView();

	fPanelsLayout = BLayoutBuilder::Cards<>()
		.Add(fRepositoryViewScroll)
		.Add(fChangesView)
		.Add(fLogView)
		.SetVisibleItem((int32)kPanelsIndexRepository);

	auto repositoryLayout = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_BORDER_INSETS)
		.SetInsets(0)
		.AddGroup(B_VERTICAL)
			.SetInsets(5)
			.Add(fBranchMenu)
			.End()
		.Add(fToolBar)
		.Add(fPanelsLayout)
		.View();

	fMainLayout = BLayoutBuilder::Cards<>()
		.Add(repositoryLayout)
		.Add(fRepositoryNotInitializedView)
		.SetVisibleItem((int32)kMainIndexRepository);

	fToolBar->ToggleActionPressed(MsgShowRepositoryPanel);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_BORDER_INSETS)
		.SetInsets(0)
		.AddGroup(B_VERTICAL)
			.SetInsets(5)
			.Add(fProjectMenu)
			.End()
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
	delete fRepositoryNotInitializedView;
}


void
SourceControlPanel::_InitToolBar()
{
	fToolBar = new ToolBar();
	fToolBar->ChangeIconSize(16);
	fToolBar->AddAction(MsgShowRepositoryPanel, B_TRANSLATE("Repository"), "kIconGitRepo", true);
	// fToolBar->AddAction(MsgShowChangesPanel, B_TRANSLATE("Changes"), "kIconGitChanges");
	// fToolBar->AddAction(MsgShowLogPanel, B_TRANSLATE("Log"), "kIconGitLog");
	fToolBar->AddGlue();
	fToolBar->AddAction(MsgShowActionsMenu, B_TRANSLATE("Actions"), "kIconGitMore", true);
}


void
SourceControlPanel::_InitRepositoryView()
{
	fRepositoryView = new RepositoryView();
	fRepositoryViewScroll = new BScrollView("Repository scroll view",
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
SourceControlPanel::_InitRepositoryNotInitializedView()
{
	auto stringView = new BStringView("InitMessage",
		B_TRANSLATE("This project does not have a git repository.\n"
			"Click below to initialize it."));
	fInitializeButton = new BButton(B_TRANSLATE("Init repository"),
		new BMessage(MsgInitializeRepository));
	fDoNotCreateInitialCommitCheckBox = new BCheckBox(B_TRANSLATE("Do not create the initial commit"),
		nullptr);
	stringView->SetAlignment(B_ALIGN_CENTER);
	fRepositoryNotInitializedView = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_BORDER_INSETS)
		.AddGroup(B_VERTICAL)
			.SetInsets(20)
			.AddGlue()
			.Add(stringView)
			.Add(fInitializeButton)
			.Add(fDoNotCreateInitialCommitCheckBox)
			.AddGlue()
		.End()
		.AddGlue()
		.View();

	// TODO: These should not be needed but without them, the splitview which separates
	// the editor from the left pane doesn't move at all
	fRepositoryNotInitializedView->SetExplicitMinSize(BSize(0, B_SIZE_UNSET));
	fRepositoryNotInitializedView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


void
SourceControlPanel::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		Window()->StartWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);
		if (gMainWindow != nullptr) {
			auto projectBrowser = gMainWindow->GetProjectBrowser();
			if (projectBrowser != nullptr)
				projectBrowser->StartWatching(this, B_PATH_MONITOR);
		}
		be_app->StartWatching(this, gCFG.UpdateMessageWhat());
		Window()->UnlockLooper();
	}

	fProjectMenu->SetTarget(this);
	fBranchMenu->SetTarget(this);
	fToolBar->SetTarget(this);
	fInitializeButton->SetTarget(this);
}


void
SourceControlPanel::DetachedFromWindow()
{
	if (Window()->LockLooper()) {
		Window()->StopWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		Window()->StopWatching(this, MSG_NOTIFY_PROJECT_SET_ACTIVE);

		if (gMainWindow != nullptr) {
			auto projectBrowser = gMainWindow->GetProjectBrowser();
			if (projectBrowser != nullptr)
				projectBrowser->StopWatching(this, B_PATH_MONITOR);
		}
		be_app->StopWatching(this, gCFG.UpdateMessageWhat());
		Window()->UnlockLooper();
	}

	BView::DetachedFromWindow();
}


void
SourceControlPanel::MessageReceived(BMessage *message)
{
	try {
		switch (message->what) {
			case B_OBSERVER_NOTICE_CHANGE: {
				int32 code;
				message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
				if (code == gCFG.UpdateMessageWhat()) {
					BString key;
					if (message->FindString("key", &key) == B_OK
						&& key == "repository_outline") {
						const ProjectFolderList* projectList = gMainWindow->GetProjectBrowser()->GetProjectList();
						if (!projectList->empty()) {
							_UpdateBranchListMenu(false);
							_UpdateRepositoryView();
						}
					}
					break;
				}
				switch (code) {
					case MSG_NOTIFY_PROJECT_LIST_CHANGED:
					{
						LogInfo("MSG_NOTIFY_PROJECT_LIST_CHANGED");
						const ProjectFolderList* projectList = gMainWindow->GetProjectBrowser()->GetProjectList();
						if (projectList->empty()) {
							fProjectMenu->MakeEmpty();
							fBranchMenu->MakeEmpty();
							fRepositoryView->MakeEmpty();
							fMainLayout->SetVisibleItem(kPanelsIndexRepository);
						}
						_UpdateProjectMenu();
						break;
					}
					case MSG_NOTIFY_PROJECT_SET_ACTIVE:
					{
						LogInfo("MSG_NOTIFY_PROJECT_SET_ACTIVE");
						BString selectedProjectName;
						BMenuItem* item = fProjectMenu->Menu()->FindMarked();
						if (item != nullptr) {
							selectedProjectName = item->Label();
						}

						BString activeProjectName = message->GetString("active_project_name");
						bool changed = false;
						if (::strcmp(activeProjectName, selectedProjectName) != 0) {
							BMenuItem* item = fProjectMenu->Menu()->FindItem(activeProjectName);
							if (item != nullptr) {
								changed = !item->IsMarked();
								item->SetMarked(true);
							}
						}
						
						if (changed) {
							ASSERT(_SelectedProject() != nullptr);
							_CheckProjectGitRepo(_SelectedProject());
							_UpdateBranchListMenu(true);
						}
						break;
					}
					case B_PATH_MONITOR:
					{
						if (gMainWindow->GetProjectBrowser()->GetProjectList()->empty())
							break;

						const ProjectFolder* selected = _SelectedProject();
						if (selected == nullptr || selected->IsBuilding())
							break;

						LogInfo("B_PATH_MONITOR");
						BString watchedPath;
						if (message->FindString("watched_path", &watchedPath) != B_OK)
							break;
						const BString projectPath = selected->Path();
						if (watchedPath != projectPath)
							break;
						// check if the project folder still exists
						if (!BEntry(projectPath).Exists())
							break;
						BString path;
						if (message->FindString("path", &path) != B_OK)
							break;

						BPath gitFolder(projectPath);
						gitFolder.Append(".git");

						if (path.FindFirst(gitFolder.Path()) == B_ERROR)
							break;

						// TODO: Move the burst handler to a function
						if (fBurstHandler == nullptr ||
								fBurstHandler->SetInterval(kBurstTimeout) != B_OK) {
							LogInfo("SourceControlPanel: fBurstHandler not valid");

							delete fBurstHandler;

							// create a message to update the project
							BMessage message(MsgChangeProject);
							message.AddString("value", selected->Path());
							message.AddString("sender", kSenderExternalEvent);
							fBurstHandler = new (std::nothrow) BMessageRunner(BMessenger(this),
								&message, kBurstTimeout, 1);
							if (fBurstHandler == nullptr|| fBurstHandler->InitCheck() != B_OK) {
								LogInfo("SourceControlPanel: Could not create "
									"fBurstHandler. Deleting it");
								delete fBurstHandler;
								fBurstHandler = nullptr;
							} else {
								LogInfo("SourceControlPanel: fBurstHandler instantiated.");
							}
						}
						break;
					}
					default:
						break;
				}
				break;
			}
			case MsgShowRepositoryPanel:
			{
				LogInfo("MsgShowRepositoryPanel");
				if (fPanelsLayout->VisibleIndex() != kPanelsIndexRepository)
					fPanelsLayout->SetVisibleItem(kPanelsIndexRepository);
				fToolBar->ToggleActionPressed(MsgShowRepositoryPanel);
				break;
			}
			case MsgShowChangesPanel:
			{
				LogInfo("MsgShowChangesPanel");
				if (fPanelsLayout->VisibleIndex() != kPanelsIndexChanges)
					fPanelsLayout->SetVisibleItem(kPanelsIndexChanges);
				fToolBar->ToggleActionPressed(MsgShowChangesPanel);
				break;
			}
			case MsgShowLogPanel:
			{
				LogInfo("MsgShowLogPanel");
				if (fPanelsLayout->VisibleIndex() != kPanelsIndexLog)
					fPanelsLayout->SetVisibleItem(kPanelsIndexLog);
				fToolBar->ToggleActionPressed(MsgShowLogPanel);
				break;
			}
			case MsgShowActionsMenu:
			{
				auto button = fToolBar->FindButton(MsgShowActionsMenu);
				auto where = button->Frame().LeftBottom();
				LogInfo("MsgShowActionsMenu: p(%f, %f)", where.x, where.y);
				_ShowOptionsMenu(where);
				break;
			}
			case MsgFetch:
			{
				LogInfo("MsgFetch");
				const ProjectFolder* selectedProject = _SelectedProject();
				if (selectedProject == nullptr)
					break;
				selectedProject->GetRepository()->Fetch();
				_ShowGitNotification(B_TRANSLATE("Fetch completed."));
				_UpdateBranchListMenu();
				break;
			}
			case MsgFetchPrune:
			{
				LogInfo("MsgFetchPrune");
				const ProjectFolder* selectedProject = _SelectedProject();
				if (selectedProject == nullptr)
					break;
				selectedProject->GetRepository()->Fetch(true);
				_ShowGitNotification(B_TRANSLATE("Fetch prune completed."));
				_UpdateBranchListMenu();
				break;
			}
			case MsgStashSave:
			{
				LogInfo("MsgStashSave");
				const ProjectFolder* selectedProject = _SelectedProject();
				if (selectedProject == nullptr)
					break;
				BString stashMessage;
				stashMessage << "WIP on " << fCurrentBranch << B_UTF8_ELLIPSIS;
				auto alert = new GTextAlert("Stash", B_TRANSLATE("Enter a message for this stash"),
					stashMessage, false);
				auto result = alert->Go();
				if (result.Button == GAlertButtons::OkButton) {
					stashMessage = result.Result;
					selectedProject->GetRepository()->StashSave(stashMessage);
					_ShowGitNotification(B_TRANSLATE("Changes stashed."));
				}
				break;
			}
			case MsgStashPop:
			{
				LogInfo("MsgStashPop");
				const ProjectFolder* selectedProject = _SelectedProject();
				if (selectedProject == nullptr)
					break;
				selectedProject->GetRepository()->StashPop();
				_ShowGitNotification(B_TRANSLATE("Stashed changes popped."));
				break;
			}
			case MsgStashApply:
			{
				LogInfo("MsgStashApply");
				const ProjectFolder* selectedProject = _SelectedProject();
				if (selectedProject == nullptr)
					break;
				selectedProject->GetRepository()->StashApply();
				_ShowGitNotification(B_TRANSLATE("Stashed changes applied."));
				break;
			}
			case MsgChangeProject:
			{
				_ChangeProject(message);
				break;
			}
			case MsgSwitchBranch:
			{
				_SwitchBranch(message);
				break;
			}
			case MsgRenameBranch:
			{
				const ProjectFolder* selectedProject = _SelectedProject();
				if (selectedProject == nullptr)
					break;
				BString selectedBranch = message->GetString("value");
				git_branch_t branchType = static_cast<git_branch_t>(message->GetInt32("type",-1));
				auto alert = new GTextAlert(B_TRANSLATE("Rename branch"),
					B_TRANSLATE("Rename branch:"), selectedBranch);
				auto result = alert->Go();
				if (result.Button == GAlertButtons::OkButton) {
					auto repo = selectedProject->GetRepository();
					repo->RenameBranch(selectedBranch, result.Result, branchType);
					_UpdateBranchListMenu();
					LogInfo("MsgRenameBranch: %s renamed to %s", selectedBranch.String(),
						result.Result.String());
				}
				break;
			}
			case MsgDeleteBranch:
			{
				const ProjectFolder* selectedProject = _SelectedProject();
				if (selectedProject == nullptr)
					break;
				const BString selectedBranch = message->GetString("value");
				StringFormatter fmt;
				fmt.Substitutions["%branch%"] = selectedBranch;

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
					git_branch_t branchType = static_cast<git_branch_t>(message->GetInt32("type",-1));
					auto repo = selectedProject->GetRepository();
					repo->DeleteBranch(selectedBranch, branchType);
					_UpdateBranchListMenu();
					LogInfo("MsgDeleteBranch: %s", selectedBranch.String());
				}
				break;
			}
			case MsgNewBranch:
			{
				const ProjectFolder* selectedProject = _SelectedProject();
				if (selectedProject == nullptr)
					break;
				const BString selectedBranch = message->GetString("value");
				BString newLocalName(selectedBranch);
				newLocalName.RemoveAll("origin/");
				git_branch_t branchType = static_cast<git_branch_t>(message->GetInt32("type", -1));
				auto alert = new GTextAlert(B_TRANSLATE("Create branch"),
					B_TRANSLATE("New branch name:"), selectedBranch);
				auto result = alert->Go();
				if (result.Button == GAlertButtons::OkButton) {
					auto repo = selectedProject->GetRepository();
					repo->CreateBranch(selectedBranch, branchType, result.Result);
					LogInfo("MsgNewBranch: %s created from %s", selectedBranch.String(),
						result.Result.String());
					GMessage switchMessage{
						{"what", MsgSwitchBranch},
						{"value", result.Result},
						{"type", branchType},
						{"sender", kSenderRepositoryPopupMenu}};
					_SwitchBranch(&switchMessage);
				}
				break;
			}
			case MsgInitializeRepository:
			{
				const ProjectFolder* selectedProject = _SelectedProject();
				if (selectedProject == nullptr)
					break;
				auto repo = selectedProject->GetRepository();
				if (!repo->IsInitialized()) {
					auto createInitialCommit = !IsChecked<BCheckBox>(fDoNotCreateInitialCommitCheckBox);
					if (!createInitialCommit) {
						BAlert* alert = new BAlert(B_TRANSLATE("Create initial commit"),
							B_TRANSLATE("Do you really want to initialize a repository without "
								"creating an initial commit?"),
							B_TRANSLATE("Cancel"), B_TRANSLATE("OK"), nullptr,
							B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
						alert->SetShortcut(0, B_ESCAPE);
						int32 choice = alert->Go();
						if (choice == 0)
							return;
					}
					selectedProject->InitRepository(createInitialCommit);
					SetChecked<BCheckBox>(fDoNotCreateInitialCommitCheckBox, false);
					BMessage message(MsgChangeProject);
					message.AddString("value", selectedProject->Path());
					message.AddString("sender", kSenderInitializeRepositoryButton);
					BMessenger(this).SendMessage(&message);
				}
				break;
			}
			case MsgCopyRefName:
			{
				const BString selectedBranch = message->GetString("value");
				BMessage* clip = nullptr;
				if (be_clipboard->Lock()) {
					be_clipboard->Clear();
					clip = be_clipboard->Data();
					if (clip != nullptr) {
						clip->AddData("text/plain", B_MIME_TYPE, selectedBranch,
									selectedBranch.Length());
						be_clipboard->Commit();
					}
					be_clipboard->Unlock();
				}
				break;
			}
			// Pull must be fixed and is disabled
			// case MsgPull: {
				// auto selectedBranch = BString(message->GetString("selectedBranch"));
				// auto result = fSelectedProjectName->GetRepository()->Pull(selectedBranch);
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
				// LogInfo("MsgPull: %s", selectedBranch.String());
				// break;
			// }
			// Pull rebase is seriously flawed. We disable this until we get it work properly
			// case MsgPullRebase: {
				// auto selectedBranch = BString(message->GetString("selectedBranch"));
				// fSelectedProjectName->GetRepository()->PullRebase();
				// _ShowGitNotification(B_TRANSLATE("Pull rebase completed."));
				// LogInfo("MsgPullRebase: %s", selectedBranch.String());
				// break;
			// }
			default:
				BView::MessageReceived(message);
				break;
		}
	} catch (const GitConflictException &ex) {
		// TODO: This is not correct: we cannot translate a non-constant expression
		auto alert = new GitAlert(B_TRANSLATE("Conflicts"),
			B_TRANSLATE(ex.Message().String()), ex.GetFiles());
		alert->Go();
		// in case of conflicts the branch will not change but the item in the OptionList will so
		// we ask the OptionList to redraw
		_UpdateBranchListMenu(false);
	} catch (const GitException &ex) {
		OKAlert("SourceControlPanel", ex.Message().String(), B_STOP_ALERT);
		_UpdateProjectMenu();
	} catch (const std::exception &ex) {
		OKAlert("SourceControlPanel", ex.what(), B_STOP_ALERT);
	} catch (...) {
		OKAlert("SourceControlPanel", B_TRANSLATE("An unknown error occurred."), B_STOP_ALERT);
	}
}


void
SourceControlPanel::_ChangeProject(BMessage *message)
{
	BString projectPath = message->GetString("value");
	const BString sender = message->GetString("sender");
	const ProjectFolder* selectedProject = gMainWindow->GetProjectBrowser()->ProjectByPath(projectPath);
	if (selectedProject != nullptr) {
		// check if the project folder still exists
		if (!BEntry(selectedProject->EntryRef()).Exists())
			return;
		// Check if the selected project is a valid git repository
		auto repo = selectedProject->GetRepository();
		if (repo->IsInitialized()) {
			if (sender == kSenderInitializeRepositoryButton ||
				sender == kSenderProjectOptionList ||
				sender == kSenderExternalEvent) {
				try {
					_UpdateBranchListMenu(false);
					_UpdateRepositoryView();
				} catch(const GitException &ex) {
					LogInfo(" %s repository has no valid info", selectedProject->Name().String());
				}
				fMainLayout->SetVisibleItem(kMainIndexRepository);
			}
		} else {
			fMainLayout->SetVisibleItem(kMainIndexInitialize);
		}
		LogInfo("Project changed to %s", selectedProject->Name().String());
	} else {
		LogError("Fatal error: ProjectFolder not valid");
	}
}


const ProjectFolder*
SourceControlPanel::_SelectedProject() const
{
	if (gMainWindow == nullptr)
		return nullptr;

	ProjectBrowser* projectBrowser = gMainWindow->GetProjectBrowser();
	if (projectBrowser == nullptr)
		return nullptr;

	BMenuItem* item = fProjectMenu->Menu()->FindMarked();
	if (item == nullptr)
		return nullptr;

	BString projectPath;
	item->Message()->FindString("value", &projectPath);

	return projectBrowser->ProjectByPath(projectPath);
}


void
SourceControlPanel::_UpdateRepositoryView()
{
	const ProjectFolder* project = _SelectedProject();
	if (project != nullptr)
		fRepositoryView->UpdateRepository(project, fCurrentBranch);
}


void
SourceControlPanel::_SwitchBranch(BMessage *message)
{
	const ProjectFolder* project = _SelectedProject();
	if (project->IsBuilding()) {
		OKAlert("Source control panel",
			B_TRANSLATE("The project is building, changing branch not allowed."),
			B_STOP_ALERT);
	} else {
		const BString branch = message->GetString("value");
		const BString sender = message->GetString("sender");

		auto repo = project->GetRepository();
		repo->SwitchBranch(branch);
		fCurrentBranch = repo->GetCurrentBranch();

		if (sender == kSenderBranchOptionList) {
			// we update the repository view
			_UpdateRepositoryView();
		} else if (sender == kSenderRepositoryPopupMenu || sender == kSenderExternalEvent) {
			// we update the repository view and the branch option list
			_UpdateBranchListMenu(false);
			_UpdateRepositoryView();
		}
	}
}


void
SourceControlPanel::_UpdateProjectMenu()
{
	// The logic here: save the currently selected project, empty the list
	// then rebuild the list and try to reselect the previously selected project.
	// otherwise select the active project.

	BMenu* projectMenu = fProjectMenu->Menu();

	BString selectedProject;
	BMenuItem* item = projectMenu->FindMarked();
	if (item != nullptr) {
		selectedProject = item->Label();
	}

	Window()->BeginViewTransaction();

	projectMenu->RemoveItems(0, projectMenu->CountItems(), true);

	fProjectMenu->SetTarget(this);
	fProjectMenu->SetSender(kSenderProjectOptionList);

	const ProjectFolderList* projectList = gMainWindow->GetProjectBrowser()->GetProjectList();
	for (size_t i = 0; i < projectList->size(); i++) {
		ProjectFolder* project = projectList->at(i);
		fProjectMenu->AddItem(project->Name(), project->Path(), MsgChangeProject);
		if (project->Name() == selectedProject)
			item->SetMarked(true);
	}
	
	if (projectMenu->FindMarked() == nullptr) {
		const ProjectFolder* activeProject = gMainWindow->GetActiveProject();
		if (activeProject != nullptr) {
			BMenuItem* item = projectMenu->FindItem(activeProject->Name());
			if (item != nullptr)
				item->SetMarked(true);
		} else {
			BMenuItem *item = projectMenu->ItemAt(0);
			if (item != nullptr)
				item->SetMarked(true);
		}
	}

	Window()->EndViewTransaction();

	const ProjectFolder* project = _SelectedProject();
	if (project != nullptr)
		_CheckProjectGitRepo(project);
}


void
SourceControlPanel::_CheckProjectGitRepo(const ProjectFolder* project)
{
	// Check if the selected project is a valid git repository
	// TODO: Rename or split this method since it not only does a check
	// but it also it changes the view as a side effect

	try {
		GitRepository* repo = project->GetRepository();
		if (repo->IsInitialized()) {
			_UpdateBranchListMenu();
		} else {
			fMainLayout->SetVisibleItem(kMainIndexInitialize);
		}
	} catch (const GitException &ex) {
		LogError("_CheckProjectGitRepo(): %s", ex.Message().String());
		throw;
	}
}


void
SourceControlPanel::_UpdateBranchListMenu(bool invokeItemMessage)
{
	try {
		const ProjectFolder* selectedProject = _SelectedProject();
		if (selectedProject != nullptr) {
			auto repo = selectedProject->GetRepository();
			if (repo->IsInitialized()) {
				auto branches = repo->GetBranches();
				fCurrentBranch = repo->GetCurrentBranch();
				LogInfo("current branch is set to %s", fCurrentBranch.String());
				fBranchMenu->SetTarget(this);
				fBranchMenu->SetSender(kSenderBranchOptionList);
				fBranchMenu->MakeEmpty();
				fBranchMenu->AddIterator(branches,
					MsgSwitchBranch,
					[](auto &item) { return item; },
					invokeItemMessage,
					[&currentBranch = fCurrentBranch](auto &item) { return (item == currentBranch);}
				);
			}
		}
	} catch(const GitException &ex) {
		fBranchMenu->MakeEmpty();
		fCurrentBranch = "";
	}
}


void
SourceControlPanel::_ShowOptionsMenu(BPoint where)
{
	StringFormatter fmt;
	fmt.Substitutions["%current_branch%"] = fCurrentBranch;

	auto optionsMenu = new BPopUpMenu("Actions", false, false);
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
	const ProjectFolder* project = _SelectedProject();
	if (project == nullptr)
		return;
	ShowNotification("Genio", project->Path().String(),
		project->Path().String(),
		text);
}
