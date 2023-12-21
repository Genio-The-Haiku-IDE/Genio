/*
 * Copyright 2022-2023 Nexus6 
 * Copyright 2017..2018 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GenioWindow.h"

#include <cassert>
#include <string>

#include <Alert.h>
#include <Application.h>
#include <Architecture.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <DirMenu.h>
#include <FilePanel.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>

#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <RecentItems.h>
#include <Resources.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <Screen.h>
#include <StringFormat.h>
#include <StringItem.h>
#include <ControlLook.h>
#include <Bitmap.h>

#include "ActionManager.h"
#include "ConfigManager.h"
#include "ConfigWindow.h"
#include "ConsoleIOView.h"
#include "ConsoleIOThread.h"
#include "EditorKeyDownMessageFilter.h"
#include "EditorMessages.h"
#include "EditorTabManager.h"
#include "FSUtils.h"
#include "GenioApp.h"
#include "GenioNamespace.h"
#include "GenioWindowMessages.h"
#include "GitAlert.h"
#include "GitRepository.h"
#include "GoToLineWindow.h"
#include "GSettings.h"
#include "IconMenuItem.h"
#include "Languages.h"
#include "Log.h"
#include "ProblemsPanel.h"
#include "ProjectSettingsWindow.h"
#include "ProjectFolder.h"
#include "ProjectItem.h"
#include "ProjectsFolderBrowser.h"
#include "QuitAlert.h"
#include "RemoteProjectWindow.h"
#include "SearchResultPanel.h"
#include "SourceControlPanel.h"
#include "SwitchBranchMenu.h"
#include "TemplatesMenu.h"
#include "TemplateManager.h"
#include "TextUtils.h"
#include "Utils.h"
#include "argv_split.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioWindow"

GenioWindow* gMainWindow = nullptr;

constexpr auto kRecentFilesNumber = 14 + 1;


// Find group
static constexpr auto kFindReplaceMaxBytes = 50;
static constexpr auto kFindReplaceMinBytes = 32;
static constexpr float kFindReplaceOPSize = 120.0f;
static constexpr auto kFindReplaceMenuItems = 10;

static float kProjectsWeight  = 1.0f;
static float kEditorWeight  = 3.14f;
static float kOutputWeight  = 0.4f;

BRect dirtyFrameHack;

static float kDefaultIconSize = 32.0;

GenioWindow::GenioWindow(BRect frame)
	:
	BWindow(frame, "Genio", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS |
												B_QUIT_ON_WINDOW_CLOSE)
	, fMenuBar(nullptr)
	, fLineEndingsMenu(nullptr)
	, fLanguageMenu(nullptr)
	, fBookmarksMenu(nullptr)
	, fBookmarkToggleItem(nullptr)
	, fBookmarkClearAllItem(nullptr)
	, fBookmarkGoToNextItem(nullptr)
	, fBookmarkGoToPreviousItem(nullptr)
	, fBuildModeItem(nullptr)
	, fReleaseModeItem(nullptr)
	, fDebugModeItem(nullptr)
	, fMakeCatkeysItem(nullptr)
	, fMakeBindcatalogsItem(nullptr)
	, fGitMenu(nullptr)
	, fGitBranchItem(nullptr)
	, fGitLogItem(nullptr)
	, fGitLogOnelineItem(nullptr)
	, fGitPullItem(nullptr)
	, fGitPullRebaseItem(nullptr)
	, fGitShowConfigItem(nullptr)
	, fGitStatusItem(nullptr)
	, fGitStatusShortItem(nullptr)
	, fGitTagItem(nullptr)
	, fToolBar(nullptr)
	, fRootLayout(nullptr)
	, fEditorTabsGroup(nullptr)
	, fProjectsTabView(nullptr)
	, fProjectsFolderBrowser(nullptr)
	, fProjectsFolderScroll(nullptr)
	, fActiveProject(nullptr)
	, fIsBuilding(false)
	, fTabManager(nullptr)
	, fFindGroup(nullptr)
	, fReplaceGroup(nullptr)
	, fFindMenuField(nullptr)
	, fReplaceMenuField(nullptr)
	, fFindTextControl(nullptr)
	, fReplaceTextControl(nullptr)
	, fFindCaseSensitiveCheck(nullptr)
	, fFindWholeWordCheck(nullptr)
	, fFindWrapCheck(nullptr)
	, fRunConsoleProgramGroup(nullptr)
	, fRunConsoleProgramText(nullptr)
	, fRunConsoleProgramButton(nullptr)
	, fConsoleStdinLine("")
	, fOpenPanel(nullptr)
	, fSavePanel(nullptr)
	, fOpenProjectPanel(nullptr)
	, fOpenProjectFolderPanel(nullptr)
	, fOutputTabView(nullptr)
	, fProblemsPanel(nullptr)
	, fBuildLogView(nullptr)
	, fConsoleIOView(nullptr)
	, fGoToLineWindow(nullptr)
	, fSearchResultPanel(nullptr)
	, fScreenMode(kDefault)
	, fDisableProjectNotifications(false)
{
	gMainWindow = this;

	_InitActions();
	_InitMenu();
	_InitWindow();

	_UpdateTabChange(nullptr, "GenioWindow");

	// Shortcuts
	for (int32 index = 1; index < 10; index++) {
		constexpr auto kAsciiPos {48};
		BMessage* selectTab = new BMessage(MSG_SELECT_TAB);
		selectTab->AddInt32("index", index - 1);
		AddShortcut(index + kAsciiPos, B_COMMAND_KEY, selectTab);
	}

	AddCommonFilter(new KeyDownMessageFilter(MSG_FILE_PREVIOUS_SELECTED, B_LEFT_ARROW, B_OPTION_KEY));
	AddCommonFilter(new KeyDownMessageFilter(MSG_FILE_NEXT_SELECTED, B_RIGHT_ARROW, B_OPTION_KEY));
	AddCommonFilter(new KeyDownMessageFilter(MSG_ESCAPE_KEY, B_ESCAPE, 0, B_DISPATCH_MESSAGE));
	AddCommonFilter(new KeyDownMessageFilter(MSG_FIND_INVOKED, B_ENTER, 0, B_DISPATCH_MESSAGE));
	AddCommonFilter(new EditorKeyDownMessageFilter());

	// Load workspace - reopen projects
	// Disable MSG_NOTIFY_PROJECT_SET_ACTIVE and MSG_NOTIFY_PROJECT_LIST_CHANGE while we populate
	// the workspace
	// TODO: improve how projects are loaded and notices are sent over
	if (gCFG["reopen_projects"]) {
		fDisableProjectNotifications = true;
		GSettings projects(GenioNames::kSettingsProjectsToReopen, 'PRRE');
		if (!projects.IsEmpty()) {
			BString projectName;
			BString activeProject = projects.GetString("active_project");
			for (auto count = 0; projects.FindString("project_to_reopen",
										count, &projectName) == B_OK; count++) {
				const BPath projectPath(projectName);
				_ProjectFolderOpen(projectPath, projectName == activeProject);
			}
		}
		// TODO: _ProjectFolderOpen() could fail to open projects, so
		// - We should check its return value
		// - here we could have no active projects or no projects at all
		fDisableProjectNotifications = false;
		SendNotices(MSG_NOTIFY_PROJECT_LIST_CHANGED);
		BMessage noticeMessage(MSG_NOTIFY_PROJECT_SET_ACTIVE);
		noticeMessage.AddPointer("active_project", fActiveProject);
		noticeMessage.AddString("active_project_name", fActiveProject ? fActiveProject->Name() : "");
		SendNotices(MSG_NOTIFY_PROJECT_SET_ACTIVE, &noticeMessage);

	}

	// Reopen files
	if (gCFG["reopen_files"]) {
		GSettings files(GenioNames::kSettingsFilesToReopen, 'FIRE');
		if (!files.IsEmpty()) {
			entry_ref ref;
			int32 index = -1, count;
			BMessage *message = new BMessage(B_REFS_RECEIVED);

			if (files.FindInt32("opened_index", &index) == B_OK) {
				message->AddInt32("opened_index", index);

				for (count = 0; files.FindRef("file_to_reopen", count, &ref) == B_OK; count++)
					message->AddRef("refs", &ref);
				// Found an index and found some files, post message
				if (index > -1 && count > 0)
					PostMessage(message);
			}
		}
	}
}


void
GenioWindow::Show()
{
	BWindow::Show();

	if (LockLooper()) {
		_ShowView(fProjectsTabView, gCFG["show_projects"], MSG_SHOW_HIDE_PROJECTS);
		_ShowView(fOutputTabView,   gCFG["show_output"],	MSG_SHOW_HIDE_OUTPUT);
		_ShowView(fToolBar,         gCFG["show_toolbar"],	MSG_TOGGLE_TOOLBAR);

		ActionManager::SetPressed(MSG_WHITE_SPACES_TOGGLE, gCFG["show_white_space"]);
		ActionManager::SetPressed(MSG_LINE_ENDINGS_TOGGLE, gCFG["show_line_endings"]);

		be_app->StartWatching(this, gCFG.UpdateMessageWhat());
		UnlockLooper();
	}
}


GenioWindow::~GenioWindow()
{
	delete fTabManager;
	delete fOpenPanel;
	delete fSavePanel;
	delete fOpenProjectFolderPanel;
	gMainWindow = nullptr;
}


void
GenioWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	//TODO: understand this part of code and move it to a better place.
	/*if (handler == fConsoleIOView) {
		if (message->what == B_KEY_DOWN) {
			int8 key;
			if (message->FindInt8("byte", 0, &key) == B_OK) {
				// A little hack to make Console I/O pipe act as line-input
				fConsoleStdinLine << static_cast<const char>(key);
				fConsoleIOView->ConsoleOutputReceived(1, (const char*)&key);
				if (key == B_RETURN) {
					fConsoleIOThread->PushInput(fConsoleStdinLine);
					fConsoleStdinLine = "";
				}
			}
		}
	}*/
	BWindow::DispatchMessage(message, handler);
}


void
GenioWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kApplyFix:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				int32 index = _GetEditorIndex(&ref);
				if (index >= 0) {
					Editor* editor = fTabManager->EditorAt(index);
					PostMessage(message, editor);
				}
			}
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			if (code == gCFG.UpdateMessageWhat()) {
				_HandleConfigurationChanged(message);
			}
			break;
		}
		case MSG_ESCAPE_KEY:
		{
			if (CurrentFocus() == fFindTextControl->TextView()) {
					_FindGroupShow(false);
			} else if (CurrentFocus() == fReplaceTextControl->TextView()) {
					_ReplaceGroupShow(false);
					fFindTextControl->MakeFocus(true);
			} else if (CurrentFocus() == fRunConsoleProgramText->TextView()) {
				fRunConsoleProgramGroup->SetVisible(false);
				fRunConsoleProgramText->MakeFocus(false);
				ActionManager::SetPressed(MSG_RUN_CONSOLE_PROGRAM_SHOW, false);
			}
			break;
		}
		case EDITOR_UPDATE_DIAGNOSTICS:
		{
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				Editor* editor = fTabManager->EditorBy(&ref);
				if (editor == fTabManager->SelectedEditor()) {
					fProblemsPanel->UpdateProblems(message);
				}
			}
			break;
		}
		case B_ABOUT_REQUESTED:
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;

		case B_COPY:
		case B_CUT:
		case B_PASTE:
		case B_SELECT_ALL:
			if (CurrentFocus())
				CurrentFocus()->MessageReceived(message);
			break;
		case B_NODE_MONITOR:
			_HandleNodeMonitorMsg(message);
			break;
		case B_REDO:
		{
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				if (editor->CanRedo())
					editor->Redo();
				_UpdateSavepointChange(editor, "Redo");
			}
			break;
		}
		case B_REFS_RECEIVED:
		case 'DATA': //file drag'n'drop
			_FileOpen(message);
			Activate();
			break;
		case B_SAVE_REQUESTED:
			_FileSaveAs(fTabManager->SelectedTabIndex(), message);
			break;
		case B_UNDO: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				if (editor->CanUndo())
					editor->Undo();
				_UpdateSavepointChange(editor, "Undo");
			}
			break;
		}
		case CONSOLEIOTHREAD_EXIT:
		{
			BString cmdType = message->GetString("cmd_type", "");
			if (cmdType == "build"  	  ||
				cmdType == "clean" 		  ||
				cmdType == "bindcatalogs" ||
				cmdType == "catkeys") {

				fIsBuilding = false;

				BMessage noticeMessage(MSG_NOTIFY_BUILDING_PHASE);
				noticeMessage.AddBool("building", fIsBuilding);
				SendNotices(MSG_NOTIFY_BUILDING_PHASE, &noticeMessage);

				fActiveProject->SetBuildingState(fIsBuilding);
			}
			_UpdateProjectActivation(fActiveProject != nullptr);
			break;
		}
		case EDITOR_FIND_SET_MARK:
		{
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				Editor* editor = fTabManager->EditorBy(&ref);
				if (editor == fTabManager->SelectedEditor()) {
					int32 line;
					if (message->FindInt32("line", &line) == B_OK) {
						BString text;
						text << editor->Name() << " :" << line;
						LogInfo(text.String());
					}
				}
			}
			break;
		}
		case EDITOR_FIND_NEXT_MISS:
		{
			LogInfo("Find next not found");
			break;
		}
		case EDITOR_FIND_PREV_MISS:
		{
			LogInfo("Find previous not found");
			break;
		}
		case EDITOR_FIND_COUNT:
		{
			int32 count;
			BString text;
			if (message->FindString("text_to_find", &text) == B_OK
				&& message->FindInt32("count", &count) == B_OK) {

				BString notification;
				notification << "\"" << text << "\""
					<< " occurrences found: " << count;
				LogInfo(notification.String());
			}
			break;
		}
		case EDITOR_REPLACE_ALL_COUNT:
		{
			int32 count;
			if (message->FindInt32("count", &count) == B_OK) {
				BString notification;
				notification << "Replacements done: " << count;
				LogInfo(notification.String());
			}
			break;
		}
		case EDITOR_REPLACE_ONE:
		{
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				Editor* editor = fTabManager->EditorBy(&ref);
				if (editor == fTabManager->SelectedEditor()) {
					int32 line, column;
					BString sel, repl;
					if (message->FindInt32("line", &line) == B_OK
						&& message->FindInt32("column", &column) == B_OK
						&& message->FindString("selection", &sel) == B_OK
						&& message->FindString("replacement", &repl) == B_OK) {
						BString notification;
						notification << editor->Name() << " " << line  << ":" << column
							 << " \"" << sel << "\" => \""<< repl<< "\"";

						LogInfo(notification.String());
					}
				}
			}
			break;
		}
		case EDITOR_POSITION_CHANGED:
		{
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				Editor* editor = fTabManager->EditorBy(&ref);
				if (editor == fTabManager->SelectedEditor()) {
					// Enable Cut,Copy,Paste shortcuts
					_UpdateSavepointChange(editor, "EDITOR_POSITION_CHANGED");
				}
			}
			break;
		}
		case EDITOR_UPDATE_SAVEPOINT:
		{
			entry_ref ref;
			bool modified = false;
			if (message->FindRef("ref", &ref) == B_OK &&
			    message->FindBool("modified", &modified) == B_OK) {

				Editor* editor = fTabManager->EditorBy(&ref);
				if (editor) {
					_UpdateLabel(_GetEditorIndex(&ref), modified);
					_UpdateSavepointChange(editor, "UpdateSavePoint");
				}
			}

			break;
		}
		case MSG_BOOKMARK_CLEAR_ALL:
		case MSG_BOOKMARK_GOTO_NEXT:
		case MSG_BOOKMARK_GOTO_PREVIOUS:
		case MSG_BOOKMARK_TOGGLE:
			_ForwardToSelectedEditor(message);
		break;
		case MSG_BUFFER_LOCK:
		{
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->SetReadOnly(!editor->IsReadOnly());
				_UpdateTabChange(editor, "Buffer Lock");
			}
			break;
		}
		case MSG_BUILD_MODE_DEBUG:
		{
			fActiveProject->SetBuildMode(BuildMode::DebugMode);
			fActiveProject->SaveSettings();
			_UpdateProjectActivation(fActiveProject != nullptr);
			break;
		}
		case MSG_BUILD_MODE_RELEASE:
		{
			fActiveProject->SetBuildMode(BuildMode::ReleaseMode);
			fActiveProject->SaveSettings();
			_UpdateProjectActivation(fActiveProject != nullptr);
			break;
		}
		case MSG_BUILD_PROJECT:
			_BuildProject();
			break;
		case MSG_CLEAN_PROJECT:
			_CleanProject();
			break;
		case MSG_DEBUG_PROJECT:
			_DebugProject();
			break;
		case MSG_EOL_CONVERT_TO_UNIX:
		case MSG_EOL_CONVERT_TO_DOS:
		case MSG_EOL_CONVERT_TO_MAC:
			_ForwardToSelectedEditor(message);
		break;
		case MSG_FILE_CLOSE:
			_FileRequestClose(fTabManager->SelectedTabIndex());
			break;
		case MSG_FILE_CLOSE_ALL:
			_FileCloseAll();
			break;
		case MSG_FILE_FOLD_TOGGLE:
			_ForwardToSelectedEditor(message);
			break;
		case MSG_FILE_MENU_SHOW:
		{
			/* Adapted from tabview */
			BPopUpMenu* tabMenu = new BPopUpMenu("filetabmenu", true, false);
			int tabCount = fTabManager->CountTabs();
			for (int index = 0; index < tabCount; index++) {
				BString label;
				label << index + 1 << ". " << fTabManager->TabLabel(index);
				BMenuItem* item = new BMenuItem(label.String(), nullptr);
				tabMenu->AddItem(item);
				if (index == fTabManager->SelectedTabIndex())
					item->SetMarked(true);
			}

			// Force layout to get the final menu size. InvalidateLayout()
			// did not seem to work here.
			tabMenu->AttachedToWindow();
			BRect buttonFrame = fToolBar->FindButton(MSG_FILE_MENU_SHOW)->Frame();
			BRect menuFrame = tabMenu->Frame();
			BPoint openPoint = ConvertToScreen(buttonFrame.LeftBottom());
			// Open with the right side of the menu aligned with the right
			// side of the button and a little below.
			openPoint.x -= menuFrame.Width() - buttonFrame.Width() + 2;
			openPoint.y += 20;

			BMenuItem *selected = tabMenu->Go(openPoint, false, false,
			ConvertToScreen(buttonFrame));
			if (selected) {
				selected->SetMarked(true);
				int32 index = tabMenu->IndexOf(selected);
				if (index != B_ERROR)
					fTabManager->SelectTab(index);
			}
			delete tabMenu;
			break;
		}
		case MSG_FILE_NEXT_SELECTED:
		{
			int32 index = fTabManager->SelectedTabIndex();

			if (index < fTabManager->CountTabs() - 1)
				fTabManager->SelectTab(index + 1);
			break;
		}
		case MSG_FILE_OPEN:
			fOpenPanel->Show();
			break;
		case MSG_FILE_PREVIOUS_SELECTED:
		{
			int32 index = fTabManager->SelectedTabIndex();
			if (index > 0 && index < fTabManager->CountTabs())
				fTabManager->SelectTab(index - 1);
			break;
		}
		case MSG_FILE_SAVE:
			_FileSave(fTabManager->SelectedTabIndex());
			break;
		case MSG_FILE_SAVE_AS:
		{
			Editor* editor = fTabManager->SelectedEditor();
			BEntry entry(editor->FileRef());
			entry.GetParent(&entry);
			fSavePanel->SetPanelDirectory(&entry);
			fSavePanel->Show();
			break;
		}
		case MSG_FILE_SAVE_ALL:
			_FileSaveAll();
			break;
		case MSG_VIEW_ZOOMIN:
		{
			int32 zoom = gCFG["editor_zoom"];
			if (zoom < 20) {
				zoom++;
				gCFG["editor_zoom"] = zoom;
			}
			break;
		}
		case MSG_VIEW_ZOOMOUT:
		{
			int32 zoom = gCFG["editor_zoom"];
			if (zoom > -10) {
				zoom--;
				gCFG["editor_zoom"] = zoom;
			}
			break;
		}
		case MSG_VIEW_ZOOMRESET:
			gCFG["editor_zoom"] = 0;
			break;
		case MSG_FIND_GROUP_SHOW:
			_FindGroupShow(true);
			break;
		case MSG_FIND_IN_FILES:
		{
			_FindInFiles();
			break;
		}
		case MSG_FIND_MARK_ALL:
		{
			BString textToFind(fFindTextControl->Text());
			if (!textToFind.IsEmpty()) {
				_FindMarkAll(textToFind);
			}
			break;
		}
		case MSG_FIND_MENU_SELECTED:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				BMenuItem* item = fFindMenuField->Menu()->ItemAt(index);
				fFindTextControl->SetText(item->Label());
			}
			break;
		}
		case MSG_FIND_INVOKED:
		{
			if (CurrentFocus() == fFindTextControl->TextView()) {
				const BString& text(fFindTextControl->Text());
				if (fTabManager->SelectedEditor())
					_FindNext(text, false);
				else
					_FindInFiles();

				fFindTextControl->MakeFocus(true);
			}
			break;
		}
		case MSG_FIND_NEXT:
		{
			const BString& text(fFindTextControl->Text());
			_FindNext(text, false);
			break;
		}
		case MSG_FIND_PREVIOUS:
		{
			const BString& text(fFindTextControl->Text());
			_FindNext(text, true);
			break;
		}
		case MSG_FIND_GROUP_TOGGLED:
			_FindGroupShow(fFindGroup->IsHidden());
			break;
		case MSG_GIT_COMMAND:
		{
			BString command;
			if (message->FindString("command", &command) == B_OK)
				_Git(command);
			break;
		}
		case MSG_GIT_SWITCH_BRANCH:
		{
			try {
				BString project_path = message->GetString("project_path", fActiveProject->Path().String());
				Genio::Git::GitRepository repo(project_path.String());
				BString new_branch = message->GetString("branch", nullptr);
				if (new_branch != nullptr)
					repo.SwitchBranch(new_branch);
			} catch (const Genio::Git::GitConflictException &ex) {
				BString message;
				message << B_TRANSLATE("An error occurred while switching branch:")
						<< " "
						<< ex.Message();

				auto alert = new GitAlert(B_TRANSLATE("Conflicts"),
											B_TRANSLATE(message), ex.GetFiles());
				alert->Go();
			} catch (const Genio::Git::GitException &ex) {
				BString message;
				message << B_TRANSLATE("An error occurred while switching branch:")
						<< " "
						<< ex.Message();

				OKAlert("GitSwitchBranch", message, B_STOP_ALERT);
			}
			break;
		}
		case GTLW_GO:
			_ForwardToSelectedEditor(message);
			break;
		case MSG_GOTO_LINE:
			if(fGoToLineWindow == nullptr) {
				fGoToLineWindow = new GoToLineWindow(this);
			}
			fGoToLineWindow->ShowCentered(Frame());
			break;
		case MSG_WHITE_SPACES_TOGGLE:
		{
			gCFG["show_white_space"] = !gCFG["show_white_space"];
			for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
				Editor* editor = fTabManager->EditorAt(index);
				editor->ShowWhiteSpaces(gCFG["show_white_space"]);
			}
			ActionManager::SetPressed(MSG_WHITE_SPACES_TOGGLE, gCFG["show_white_space"]);
			break;
		}
		case MSG_LINE_ENDINGS_TOGGLE:
		{
			gCFG["show_line_endings"] = !gCFG["show_line_endings"];
			for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
				Editor* editor = fTabManager->EditorAt(index);
				editor->ShowLineEndings(gCFG["show_line_endings"]);
			}
			ActionManager::SetPressed(MSG_LINE_ENDINGS_TOGGLE, gCFG["show_line_endings"]);
			break;
		}
		case MSG_DUPLICATE_LINE:
		case MSG_DELETE_LINES:
		case MSG_COMMENT_SELECTED_LINES:
		case MSG_FILE_TRIM_TRAILING_SPACE:
		case MSG_AUTOCOMPLETION:
		case MSG_FORMAT:
		case MSG_GOTODEFINITION:
		case MSG_GOTODECLARATION:
		case MSG_GOTOIMPLEMENTATION:
		case MSG_SWITCHSOURCE:
			_ForwardToSelectedEditor(message);
			break;
		case MSG_MAKE_BINDCATALOGS:
			_MakeBindcatalogs();
			break;
		case MSG_MAKE_CATKEYS:
			_MakeCatkeys();
			break;
		case MSG_PROJECT_CLOSE:
			_ProjectFolderClose(fActiveProject);
			break;
		case MSG_SHOW_TEMPLATE_USER_FOLDER:
		{
			entry_ref newRef;
			status_t status = message->FindRef("refs", &newRef);
			if (status != B_OK) {
				LogError("Can't find ref in message!");
			} else {
				_ShowInTracker(newRef);
			}
			break;
		}
		case MSG_CREATE_NEW_PROJECT:
		{
			entry_ref dest_ref;
			entry_ref template_ref;
			BString name;
			if (message->FindRef("template_ref", &template_ref) != B_OK) {
				LogError("Invalid template %s", template_ref.name);
				return;
			}
			if (message->FindRef("directory", &dest_ref) != B_OK) {
				LogError("Invalid destination directory %s", dest_ref.name);
				return;
			}
			if (message->FindString("name", &name) != B_OK) {
				LogError("Invalid destination name %s", name.String());
				return;
			}

			if (TemplateManager::CopyProjectTemplate(&template_ref, &dest_ref, name.String()) == B_OK) {
				BPath path(&dest_ref);
				path.Append(name);
				_ProjectFolderOpen(path);
			} else {
				LogError("TemplateManager: could create %s from %s to %s",
							name.String(), template_ref.name, dest_ref.name);
			}
			break;
		}
		case MSG_FILE_NEW:
		case MSG_PROJECT_MENU_NEW_FILE:
		{
			BString type;
			status_t status = message->FindString("type", &type);
			if (status != B_OK) {
				LogError("Can't find type!");
				return;
			}

			if (type == "new_folder") {
				ProjectItem* item = fProjectsFolderBrowser->GetSelectedProjectItem();
				if (item && item->GetSourceItem()->Type() != SourceItemType::FileItem) {
					const entry_ref* ref = item->GetSourceItem()->EntryRef();
					status = TemplateManager::CreateNewFolder(ref);
					if (status != B_OK) {
						OKAlert(B_TRANSLATE("New folder"),
								B_TRANSLATE("Error creating folder"),
								B_WARNING_ALERT);
						LogError("Invalid destination directory [%s]", ref->name);
					}
				} else {
					LogError("Can't find current item");
					OKAlert(B_TRANSLATE("New folder"),
							B_TRANSLATE("You can't create a new folder here, "
										"please select a project or another folder"),
							B_WARNING_ALERT);
					return;
				}
			}

			// new_folder_template corresponds to creating a new project
			// there is no need to check the selected item in the ProjectBrowser
			// A FilePanel is shown to let the user select the destination of the new project
			if (type == "new_folder_template") {
				LogTrace("new_folder_template");
				entry_ref template_ref;
				if (message->FindRef("refs", &template_ref) == B_OK) {
					BMessage *msg = new BMessage(MSG_CREATE_NEW_PROJECT);
					msg->AddRef("template_ref", &template_ref);
					fCreateNewProjectPanel->SetMessage(msg);
					fCreateNewProjectPanel->Show();
				}
			}

			// new_file_template corresponds to creating a new file
			if (type ==  "new_file_template") {
				entry_ref dest;
				entry_ref source;
				ProjectItem* item = fProjectsFolderBrowser->GetSelectedProjectItem();
				if (item && item->GetSourceItem()->Type() != SourceItemType::FileItem) {
					const entry_ref* entryRef = item->GetSourceItem()->EntryRef();
					if (message->FindRef("refs", &source) != B_OK) {
						LogError("Can't find ref in message!");
						return;
					}
					status_t status = TemplateManager::CopyFileTemplate(&source, &dest);
					if (status != B_OK) {
						OKAlert(B_TRANSLATE("New file"),
								B_TRANSLATE("Could not create a new file"),
								B_WARNING_ALERT);
						LogError("Invalid destination directory [%s]", entryRef->name);
						return;
					}
				}
			}
			break;
		}
		case MSG_PROJECT_MENU_CLOSE:
			_ProjectFolderClose(fProjectsFolderBrowser->GetProjectFromSelectedItem());
			break;
		case MSG_PROJECT_MENU_DELETE_FILE:
			_ProjectFileDelete();
			break;
		case MSG_PROJECT_MENU_RENAME_FILE:
			_ProjectRenameFile();
			break;
		case MSG_PROJECT_MENU_SHOW_IN_TRACKER:
			_ShowSelectedItemInTracker();
			break;
		case MSG_PROJECT_MENU_OPEN_TERMINAL:
			_OpenTerminalWorkingDirectory();
			break;
		case MSG_PROJECT_MENU_SET_ACTIVE:
			_ProjectFolderActivate(fProjectsFolderBrowser->GetProjectFromSelectedItem());
			break;
		case MSG_PROJECT_OPEN:
			fOpenProjectFolderPanel->Show();
			break;
		case MSG_PROJECT_OPEN_REMOTE:
		{
			const char* projectsPath = gCFG["projects_directory"];
			BEntry entry(projectsPath, true);
			BPath path;
			entry.GetPath(&path);

			RemoteProjectWindow *window = new RemoteProjectWindow("", path.Path(), BMessenger(this));
			window->Show();
			break;
		}
		case MSG_PROJECT_SETTINGS:
		{
			if (fActiveProject != nullptr) {
				ProjectSettingsWindow *window = new ProjectSettingsWindow(fActiveProject);
				window->Show();
			}
			break;
		}
		case MSG_PROJECT_FOLDER_OPEN:
			_ProjectFolderOpen(message);
			break;
		case MSG_REPLACE_GROUP_SHOW:
			_ReplaceGroupShow(true);
			break;
		case MSG_REPLACE_ALL:
			_Replace(REPLACE_ALL);
			break;
		case MSG_REPLACE_GROUP_TOGGLED:
			_ReplaceGroupShow(fReplaceGroup->IsHidden());
			break;
		case MSG_REPLACE_MENU_SELECTED:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				BMenuItem* item = fReplaceMenuField->Menu()->ItemAt(index);
				fReplaceTextControl->SetText(item->Label());
			}
			break;
		}
		case MSG_REPLACE_NEXT:
			_Replace(REPLACE_NEXT);
			break;
		case MSG_REPLACE_ONE:
			_Replace(REPLACE_ONE);
			break;
		case MSG_REPLACE_PREVIOUS:
			_Replace(REPLACE_PREVIOUS);
			break;
		case MSG_RUN_CONSOLE_PROGRAM_SHOW:
		{
			if (fRunConsoleProgramGroup->IsVisible()) {
				fRunConsoleProgramGroup->SetVisible(false);
				fRunConsoleProgramText->MakeFocus(false);
			} else {
				fRunConsoleProgramGroup->SetVisible(true);
				fRunConsoleProgramText->MakeFocus(true);
			}
			ActionManager::SetPressed(MSG_RUN_CONSOLE_PROGRAM_SHOW, fRunConsoleProgramGroup->IsVisible());
			break;
		}
		case MSG_RUN_CONSOLE_PROGRAM:
		{
			const BString& command(fRunConsoleProgramText->Text());
			if (!command.IsEmpty())
				_RunInConsole(command);
			fRunConsoleProgramText->SetText("");
			break;
		}
		case MSG_RUN_TARGET:
			_RunTarget();
			break;
		case MSG_SELECT_TAB:
		{
			int32 index;
			// Shortcut selection, be careful
			if (message->FindInt32("index", &index) == B_OK) {
				if (index < fTabManager->CountTabs()
					&& index != fTabManager->SelectedTabIndex())
					fTabManager->SelectTab(index);
			}
			break;
		}
		case MSG_SHOW_HIDE_PROJECTS:
			_ShowView(fProjectsTabView, fProjectsTabView->IsHidden(), MSG_SHOW_HIDE_PROJECTS);
			break;
		case MSG_SHOW_HIDE_OUTPUT:
			_ShowView(fOutputTabView, fOutputTabView->IsHidden(), MSG_SHOW_HIDE_OUTPUT);
			break;
		case MSG_FULLSCREEN:
		case MSG_FOCUS_MODE:
			_ToogleScreenMode(message->what);
			break;
		case MSG_TEXT_OVERWRITE:
			_ForwardToSelectedEditor(message);
			break;
		case MSG_TOGGLE_TOOLBAR:
			_ShowView(fToolBar, fToolBar->IsHidden(), MSG_TOGGLE_TOOLBAR);
			break;
		case MSG_WINDOW_SETTINGS:
		{
			ConfigWindow* window = new ConfigWindow(gCFG);
			window->Show();
			break;
		}
		case TABMANAGER_TAB_SELECTED:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				Editor* editor = fTabManager->EditorAt(index);
				// TODO notify and check index too
				if (editor == nullptr) {
					LogError("Selecting editor but it's null! (index %d)", index);
					break;
				}
				const int32 be_line   = message->GetInt32("be:line", -1);
				const int32 lsp_char  = message->GetInt32("lsp:character", -1);

				if (lsp_char >= 0 && be_line > -1) {
					editor->GoToLSPPosition(be_line - 1, lsp_char);
				} else if (be_line > -1) {
					editor->GoToLine(be_line);
				}

				editor->GrabFocus();
				_UpdateTabChange(editor, "TABMANAGER_TAB_SELECTED");
			}
			break;
		}
		case TABMANAGER_TAB_CLOSE_MULTI:
			_CloseMultipleTabs(message);
			break;
		case TABMANAGER_TAB_NEW_OPENED:
		{
			int32 index =  message->GetInt32("index", -1);
			bool set_caret = message->GetBool("caret_position", false);
			if (set_caret && index >= 0) {
				Editor* editor = fTabManager->EditorAt(index);
				if (editor)
					editor->SetSavedCaretPosition();
			}
			break;
		}
		case MSG_FIND_WRAP:
			gCFG["find_wrap"] = (bool)fFindWrapCheck->Value();
			break;
		case MSG_FIND_WHOLE_WORD:
			gCFG["find_whole_word"] = (bool)fFindWholeWordCheck->Value();
			break;
		case MSG_FIND_MATCH_CASE:
			gCFG["find_match_case"] = (bool)fFindCaseSensitiveCheck->Value();
			break;
		case MSG_SET_LANGUAGE:
			_ForwardToSelectedEditor(message);
			break;
		case MSG_HELP_GITHUB:
		{
			char *argv[2] = {(char*)"https://github.com/Genio-The-Haiku-IDE/Genio", NULL};
			be_roster->Launch("text/html", 1, argv);
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


ProjectFolder*
GenioWindow::GetActiveProject() const
{
	return fActiveProject;
}


void
GenioWindow::SetActiveProject(ProjectFolder *project)
{
	fActiveProject = project;
}


ProjectsFolderBrowser*
GenioWindow::GetProjectBrowser() const
{
	return fProjectsFolderBrowser;
}


//Freely inspired by the haiku Terminal fullscreen function.
void
GenioWindow::_ToogleScreenMode(int32 action)
{
	if (fScreenMode == kDefault) { // go fullscreen
		fScreenModeSettings.MakeEmpty();
		fScreenModeSettings["saved_frame"] = Frame();
		fScreenModeSettings["saved_look"] = (int32)Look();
		fScreenModeSettings["show_projects"] = !fProjectsTabView->IsHidden();
		fScreenModeSettings["show_output"]   = !fOutputTabView->IsHidden();
		fScreenModeSettings["show_toolbar"]  = !fToolBar->IsHidden();

		BScreen screen(this);
		fMenuBar->Hide();
		SetLook(B_NO_BORDER_WINDOW_LOOK);
		ResizeTo(screen.Frame().Width() + 1, screen.Frame().Height() + 1);
		MoveTo(screen.Frame().left, screen.Frame().top);
		SetFlags(Flags() | (B_NOT_RESIZABLE | B_NOT_MOVABLE));

		ActionManager::SetEnabled(MSG_TOGGLE_TOOLBAR, false);
		ActionManager::SetEnabled(MSG_SHOW_HIDE_PROJECTS, false);
		ActionManager::SetEnabled(MSG_SHOW_HIDE_OUTPUT, false);

		if (action == MSG_FULLSCREEN) {
			fScreenMode = kFullscreen;
		} else if (action == MSG_FOCUS_MODE) {
			_ShowView(fToolBar,         false, MSG_TOGGLE_TOOLBAR);
			_ShowView(fProjectsTabView, false, MSG_SHOW_HIDE_PROJECTS);
			_ShowView(fOutputTabView,   false, MSG_SHOW_HIDE_OUTPUT);
			fScreenMode = kFocus;
		}
	} else { // exit fullscreen
		BRect savedFrame = fScreenModeSettings["saved_frame"];
		int32 restoredLook = fScreenModeSettings["saved_look"];

		fMenuBar->Show();
		ResizeTo(savedFrame.Width(), savedFrame.Height());
		MoveTo(savedFrame.left, savedFrame.top);
		SetLook((window_look)restoredLook);
		SetFlags(Flags() & ~(B_NOT_RESIZABLE | B_NOT_MOVABLE));

		ActionManager::SetEnabled(MSG_TOGGLE_TOOLBAR, true);
		ActionManager::SetEnabled(MSG_SHOW_HIDE_PROJECTS, true);
		ActionManager::SetEnabled(MSG_SHOW_HIDE_OUTPUT, true);

		_ShowView(fToolBar,         fScreenModeSettings["show_toolbar"], MSG_TOGGLE_TOOLBAR);
		_ShowView(fProjectsTabView, fScreenModeSettings["show_projects"] , MSG_SHOW_HIDE_PROJECTS);
		_ShowView(fOutputTabView,   fScreenModeSettings["show_output"], MSG_SHOW_HIDE_OUTPUT);

		fScreenMode = kDefault;
	}
}


void
GenioWindow::_ForwardToSelectedEditor(BMessage* msg)
{
	Editor* editor = fTabManager->SelectedEditor();
	if (editor) {
		PostMessage(msg, editor);
	}
}


void
GenioWindow::_CloseMultipleTabs(BMessage* msg)
{
	type_code	typeFound;
	int32		countFound;
	bool		fixedSize;

	if (msg->GetInfo("index", &typeFound, &countFound, &fixedSize) == B_OK) {
		int32 index;
		std::vector<int32> unsavedIndex;
		for (int32 i=0; i<countFound; i++) {
			if (msg->FindInt32("index", i, &index) == B_OK) {
				Editor* editor = fTabManager->EditorAt(index);
				if (editor && editor->IsModified()) {
					unsavedIndex.push_back(index);
				}
			}
		}

		if (!_FileRequestSaveList(unsavedIndex))
			return;

		for (int32 i=0; i<countFound; i++) {
			if (msg->FindInt32("index", i, &index) == B_OK) {
				_RemoveTab(index);
			}
		}
	}
}


bool
GenioWindow::_FileRequestSaveAllModified()
{
	std::vector<int32>		 unsavedIndex;

	for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
		Editor* editor = fTabManager->EditorAt(index);
		if(editor->IsModified()) {
			unsavedIndex.push_back(index);
		}
	}

	return _FileRequestSaveList(unsavedIndex);
}


bool
GenioWindow::_FileRequestClose(int32 index)
{
	if (index < 0)
		return true;

	Editor* editor = fTabManager->EditorAt(index);
	if (editor) {
		if (editor->IsModified()) {
			std::vector<int32> unsavedIndex { index };
			if (!_FileRequestSaveList(unsavedIndex))
				return false;

		}
		_RemoveTab(index);
	}

	return true;
}


bool
GenioWindow::_FileRequestSaveList(std::vector<int32>& unsavedIndex)
{
	if (unsavedIndex.empty())
		return true;

	std::vector<std::string> unsavedPaths;
	for (int i:unsavedIndex) {
		Editor* editor = fTabManager->EditorAt(i);
		unsavedPaths.push_back(std::string(editor->FilePath().String()));
	}

	if (unsavedIndex.size() == 1) {
		BString text(B_TRANSLATE("Save changes to file \"%file%\""));
		text.ReplaceAll("%file%", unsavedPaths[0].c_str());

		BAlert* alert = new BAlert("CloseAndSaveDialog", text,
 			B_TRANSLATE("Cancel"), B_TRANSLATE("Don't save"), B_TRANSLATE("Save"),
 			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

		alert->SetShortcut(0, B_ESCAPE);

		switch (alert->Go()) {
			case 2: // Save and close.
				_FileSave(unsavedIndex[0]);
			case 1: // Don't save (close)
				return true;
			case 0: // Cancel
			default:
				return false;
		};
	}
	//Let's use Koder QuitAlert!

	QuitAlert* quitAlert = new QuitAlert(unsavedPaths);
	auto filesToSave = quitAlert->Go();

	if (filesToSave.empty()) //Cancel the request!
		return false;

	auto bter = filesToSave.begin();
	auto iter = unsavedIndex.begin();
	while (iter != unsavedIndex.end()) {
		if ((*bter)) {
			_FileSave(*iter);
		}
		iter++;
		bter++;
	}

	return true;
}


bool
GenioWindow::QuitRequested()
{
	if (!_FileRequestSaveAllModified())
		return false;

	if (fScreenMode != kDefault)
		_ToogleScreenMode(-1);

	gCFG["show_projects"] = !fProjectsTabView->IsHidden();
	gCFG["show_output"]   = !fOutputTabView->IsHidden();
	gCFG["show_toolbar"]  = !fToolBar->IsHidden();

	// Files to reopen
	if (gCFG["reopen_files"]) {
		GSettings files(GenioNames::kSettingsFilesToReopen, 'FIRE');
		// Just empty it for now TODO check if equal
		files.MakeEmpty();

		// Save if there is an opened file
		int32 index = fTabManager->SelectedTabIndex();

		if (index > -1 && index < fTabManager->CountTabs()) {
			files.AddInt32("opened_index", index);

			for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
				Editor* editor = fTabManager->EditorAt(index);
				files.AddRef("file_to_reopen", editor->FileRef());
			}
		}
		files.Save();
	}

	// remove link between all editors and all projects
	for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
		fTabManager->EditorAt(index)->SetProjectFolder(NULL);
	}

	// Projects to reopen
	if (gCFG["reopen_projects"]) {

		GSettings projects(GenioNames::kSettingsProjectsToReopen, 'PRRE');

		projects.MakeEmpty();

		for (int32 index = 0; index < GetProjectBrowser()->CountProjects(); index++) {
			ProjectFolder *project = GetProjectBrowser()->ProjectAt(index);
			projects.AddString("project_to_reopen", project->Path());
			if (project->Active())
				projects.SetString("active_project", project->Path());
			// Avoiding leaks
			//TODO:---> _ProjectOutlineDepopulate(project);
			delete project;
		}
		projects.Save();
	}

	if (fGoToLineWindow != nullptr) {
		fGoToLineWindow->LockLooper();
		fGoToLineWindow->Quit();
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


status_t
GenioWindow::_AddEditorTab(entry_ref* ref, int32 index, BMessage* addInfo)
{
	Editor* editor = new Editor(ref, BMessenger(this));
	fTabManager->AddTab(editor, ref->name, index, addInfo);

	return B_OK;
}


status_t
GenioWindow::_AlertInvalidBuildConfig(BString message)
{
	BAlert* alert = new BAlert("Building project", B_TRANSLATE(message),
						B_TRANSLATE("Cancel"),
						B_TRANSLATE("Configure"),
						nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	if (alert->Go() == 1) {
		PostMessage(MSG_PROJECT_SETTINGS);
	}
	return B_OK;
}


status_t
GenioWindow::_BuildProject()
{
	// Should not happen
	if (fActiveProject == nullptr)
		return B_ERROR;

	BString command;
	command	<< fActiveProject->GetBuildCommand();

	if (command.IsEmpty()) {
		LogInfoF("Empty build command for project [%s]", fActiveProject->Name().String());

		BString message;
		message << "No build command found!\n"
				   "Please configure the project to provide\n"
				   "a valid build configuration.";
		return _AlertInvalidBuildConfig(message);
	}

	// TODO: Should ask if the user wants to save
	if (gCFG["save_on_build"])
		_FileSaveAll(fActiveProject);

	fIsBuilding = true;

	BMessage noticeMessage(MSG_NOTIFY_BUILDING_PHASE);
	noticeMessage.AddBool("building", fIsBuilding);
	SendNotices(MSG_NOTIFY_BUILDING_PHASE, &noticeMessage);

	fActiveProject->SetBuildingState(fIsBuilding);

	_UpdateProjectActivation(false);

	fBuildLogView->Clear();
	_ShowLog(kBuildLog);

	LogInfoF("Build started: [%s]", fActiveProject->Name().String());

	BString claim("Build ");
	claim << fActiveProject->Name();
	claim << " (";
	claim << (fActiveProject->GetBuildMode() == BuildMode::ReleaseMode ? B_TRANSLATE("Release") : B_TRANSLATE("Debug"));
	claim << ")";

	GMessage message = {{"cmd", command},
						{"cmd_type", "build"},
						{"banner_claim", claim }};

	// Go to appropriate directory
	chdir(fActiveProject->Path());

	return fBuildLogView->RunCommand(&message);
}


status_t
GenioWindow::_CleanProject()
{
	// Should not happen
	if (fActiveProject == nullptr)
		return B_ERROR;

	BString command;
	command	<< fActiveProject->GetCleanCommand();

	if (command.IsEmpty()) {
		LogInfoF("Empty clean command for project [%s]", fActiveProject->Name().String());

		BString message;
		message << "No clean command found!\n"
				   "Please configure the project to provide\n"
				   "a valid clean configuration.";
		return _AlertInvalidBuildConfig(message);
	}

	_UpdateProjectActivation(false);

	fBuildLogView->Clear();
	_ShowLog(kBuildLog);

	LogInfoF("Clean started: [%s]", fActiveProject->Name().String());

	fIsBuilding = true;

	BMessage noticeMessage(MSG_NOTIFY_BUILDING_PHASE);
	noticeMessage.AddBool("building", fIsBuilding);
	SendNotices(MSG_NOTIFY_BUILDING_PHASE, &noticeMessage);

	fActiveProject->SetBuildingState(fIsBuilding);

	BString claim("Build ");
	claim << fActiveProject->Name();
	claim << " (";
	claim << (fActiveProject->GetBuildMode() == BuildMode::ReleaseMode ? B_TRANSLATE("Release") : B_TRANSLATE("Debug"));
	claim << ")";

	GMessage message = {{"cmd", command},
						{"cmd_type", "build"},
						{"banner_claim", claim }};

	// Go to appropriate directory
	chdir(fActiveProject->Path());

	return fBuildLogView->RunCommand(&message);
}


status_t
GenioWindow::_DebugProject()
{
	// Should not happen
	if (fActiveProject == nullptr)
		return B_ERROR;

	// Release mode enabled, should not happen
	// TODO: why not ?
	if (fActiveProject->GetBuildMode() == BuildMode::ReleaseMode)
		return B_ERROR;


	argv_split parser(fActiveProject->GetTarget().String());
	parser.parse(fActiveProject->GetExecuteArgs().String());

	return be_roster->Launch("application/x-vnd.Haiku-debugger", parser.getArguments().size() , parser.argv());
}


status_t
GenioWindow::_RemoveTab(int32 index)
{
	if (index < 0 || index > fTabManager->CountTabs()) {
		LogErrorF("No file selected %d", index);
		return B_ERROR;
	}
	Editor* editor = fTabManager->EditorAt(index);
	if (!editor)
		return B_ERROR;

	fTabManager->RemoveTab(index);

	// notify listeners: file could have been modified, but user
	// chose not to save
	BMessage noticeMessage(MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED);
	noticeMessage.AddString("file_name", editor->FilePath());
	noticeMessage.AddBool("needs_save", false);
	SendNotices(MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED, &noticeMessage);

	// notify listeners:
	BMessage noticeCloseMessage(MSG_NOTIFY_EDITOR_FILE_CLOSED);
	noticeCloseMessage.AddString("file_name", editor->FilePath());
	SendNotices(MSG_NOTIFY_EDITOR_FILE_CLOSED, &noticeCloseMessage);

	delete editor;

	// Was it the last one?
	if (fTabManager->CountTabs() == 0)
		_UpdateTabChange(nullptr, "_RemoveTab");

	return B_OK;
}


void
GenioWindow::_FileCloseAll()
{
	if (!_FileRequestSaveAllModified())
		return;

	int32 tabsCount = fTabManager->CountTabs();
	// If there is something to close
	if (tabsCount > 0) {
		// Don't lose time in changing selection on removal
		fTabManager->SelectTab(int32(0));

		for (int32 index = tabsCount - 1; index >= 0; index--) {
			_RemoveTab(index);
		}
	}
}


//
status_t
GenioWindow::_FileOpen(BMessage* msg)
{
	int32 nextIndex;
	// If user choose to reopen files reopen right index
	// otherwise use default behaviour (see below)
	if (msg->FindInt32("opened_index", &nextIndex) != B_OK)
		nextIndex = fTabManager->CountTabs();

	const int32 be_line   = msg->GetInt32("be:line", -1);
	const int32 lsp_char  = msg->GetInt32("lsp:character", -1);

	BMessage selectTabInfo;
	selectTabInfo.AddInt32("be:line", be_line);
	selectTabInfo.AddInt32("lsp:character", lsp_char);

	bool openWithPreferred	= msg->GetBool("openWithPreferred", false);

	status_t status = B_OK;
	entry_ref ref;
	int32 refsCount = 0;
	while (msg->FindRef("refs", refsCount++, &ref) == B_OK) {
		// Check existence
		BEntry entry(&ref);
		if (entry.Exists() == false)
			continue;
		// first let's see if it's already opened.
		const int32 openedIndex = _GetEditorIndex(&ref);
		if (openedIndex != -1) {
			if (openedIndex != fTabManager->SelectedTabIndex()) {
				fTabManager->SelectTab(openedIndex, &selectTabInfo);
			} else {
				if (lsp_char >= 0 && be_line > -1) {
					fTabManager->SelectedEditor()->GoToLSPPosition(be_line - 1, lsp_char);
				} else if (be_line > -1) {
					fTabManager->SelectedEditor()->GoToLine(be_line);
				}
				fTabManager->SelectedEditor()->GrabFocus();
			}
			continue;
		}

		if (!_FileIsSupported(&ref)) {
			if (openWithPreferred)
				_FileOpenWithPreferredApp(&ref); //TODO make this optional?
			continue;
		}

		// register the file as a recent one.
		be_roster->AddToRecentDocuments(&ref, GenioNames::kApplicationSignature);

		// new file to load..
		selectTabInfo.AddBool("caret_position", true);
		int32 index = fTabManager->CountTabs();
		if (_AddEditorTab(&ref, index, &selectTabInfo) != B_OK) {
			// Error.
			continue;
		}
		Editor* editor = fTabManager->EditorAt(index);
		assert(index >= 0 && editor);

		status = editor->LoadFromFile();
		if (status != B_OK) {
			continue;
		}

		editor->ApplySettings();

		/*
			Let's assign the right "LSPClientWrapper" to the Editor..
		*/
		// Check if already open
		BString baseDir("");
		for (int32 index = 0; index < GetProjectBrowser()->CountProjects(); index++) {
			ProjectFolder * project = GetProjectBrowser()->ProjectAt(index);
			BString projectPath = project->Path();
			projectPath = projectPath.Append("/");
			if (editor->FilePath().StartsWith(projectPath)) {
				editor->SetProjectFolder(project);
			}
		}

		fTabManager->SelectTab(index, &selectTabInfo);

		BMessage noticeMessage(MSG_NOTIFY_EDITOR_FILE_OPENED);
		noticeMessage.AddString("file_name", editor->FilePath());
		SendNotices(MSG_NOTIFY_EDITOR_FILE_OPENED, &noticeMessage);

		LogInfo("File open: %s [%d]", editor->Name().String(), fTabManager->CountTabs() - 1);
	}

	// If at least 1 item or more were added select the first
	// of them.
	if (nextIndex < fTabManager->CountTabs())
		fTabManager->SelectTab(nextIndex);

	return status;
}


bool
GenioWindow::_FileIsSupported(const entry_ref* ref)
{
	//what files are supported?
	//a bit of heuristic!

	BNode entry(ref);
	if (entry.InitCheck() != B_OK || entry.IsDirectory())
		return false;

	BPath path(ref);
	std::string fileType = "";
	if (Languages::GetLanguageForExtension(GetFileExtension(path.Path()), fileType))
		return true;

	BNodeInfo info(&entry);
	if (info.InitCheck() == B_OK) {
		char mime[B_MIME_TYPE_LENGTH + 1];
		if (info.GetType(mime) != B_OK) {
			LogError("Error in getting mime type from file [%s]", path.Path());
			mime[0] = '\0';
		}
		if (mime[0] == '\0' || strcmp(mime, "application/octet-stream") == 0) {
			if (update_mime_info(path.Path(), false, true, B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL) == B_OK) {
				if (info.GetType(mime) != B_OK) {
					LogError("Error in getting mime type from file [%s]", path.Path());
					mime[0] = '\0';
				}
			}
		}

		if (strncmp(mime, "text/", 5) == 0)
			return true;
	}
	return false;
}


status_t
GenioWindow::_FileOpenWithPreferredApp(const entry_ref* ref)
{
	BNode entry(ref);
	status_t status = entry.InitCheck();
	if (status != B_OK)
		return status;
	if (entry.IsDirectory())
		return B_IS_A_DIRECTORY;

	return be_roster->Launch(ref);
}


status_t
GenioWindow::_FileSave(int32 index)
{
	// Should not happen
	if (index < 0) {
		LogErrorF("No file selected (%d)", index);
		return B_ERROR;
	}

	Editor* editor = fTabManager->EditorAt(index);
	if (editor == nullptr) {
		LogErrorF("NULL editor pointer (%d)", index);
		return B_ERROR;
	}

	// Readonly file, should not happen
	if (editor->IsReadOnly()) {
		LogErrorF("File is read-only (%s)", editor->FilePath().String());
		return B_ERROR;
	}

	_PreFileSave(editor);

	// Stop monitoring if needed
	editor->StopMonitoring();

	ssize_t written = editor->SaveToFile();
	ssize_t length = editor->SendMessage(SCI_GETLENGTH, 0, 0);

	// Restart monitoring
	editor->StartMonitoring();

	if (length == written)
		LogInfoF("File saved! (%s) bytes(%ld) -> written(%ld)", editor->FilePath().String(), length, written);
	else
		LogErrorF("Error saving file! (%s) bytes(%ld) -> written(%ld)", editor->FilePath().String(), length, written);

	_PostFileSave(editor);

	return B_OK;
}


void
GenioWindow::_FileSaveAll(ProjectFolder* onlyThisProject)
{
	int32 filesCount = fTabManager->CountTabs();
	for (int32 index = 0; index < filesCount; index++) {
		Editor* editor = fTabManager->EditorAt(index);
		if (editor == nullptr) {
			BString notification;
			notification << "Index " << index
				<< ": " << "NULL editor pointer";
			LogInfo(notification.String());
			continue;
		}

		// If a project was specified and the file doesn't belong, skip
		if (onlyThisProject != NULL && editor->GetProjectFolder() != onlyThisProject)
			continue;

		if (editor->IsModified())
			_FileSave(index);
	}
}


status_t
GenioWindow::_FileSaveAs(int32 selection, BMessage* message)
{
	entry_ref ref;
	BString name;
	status_t status;

	if ((status = message->FindRef("directory", &ref)) != B_OK)
		return status;
	if ((status = message->FindString("name", &name)) != B_OK)
		return status;

	BPath path(&ref);
	path.Append(name);
	BEntry entry(path.Path(), true);
	entry_ref newRef;

	if ((status = entry.GetRef(&newRef)) != B_OK)
		return status;

	Editor* editor = fTabManager->EditorAt(selection);

	if (editor == nullptr) {
		BString notification;
		notification
			<< "Index " << selection << ": NULL editor pointer";
		LogInfo(notification.String());
		return B_ERROR;
	}

	editor->SetFileRef(&newRef);
	fTabManager->SetTabLabel(selection, editor->Name().String());

	/* Modified files 'Saved as' get saved to an unmodified state.
	 * It should be cool to take the modified state to the new file and let
	 * user choose to save or discard modifications. Left as a TODO.
	 * In case do not forget to update label
	 */
	//_UpdateLabel(selection, editor->IsModified());

	_FileSave(selection);

	return B_OK;
}


int32
GenioWindow::_FilesNeedSave()
{
	int32 count = 0;
	for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
		Editor* editor = fTabManager->EditorAt(index);
		if (editor->IsModified())
			count++;
	}

	return count;
}


void
GenioWindow::_PreFileSave(Editor* editor)
{
	if (gCFG["trim_trailing_whitespace"])
		editor->TrimTrailingWhitespace();
}


void
GenioWindow::_PostFileSave(Editor* editor)
{
	// TODO: Also handle cases where the file is saved from outside Genio ?
	ProjectFolder* project = editor->GetProjectFolder();
	if (gCFG["build_on_save"] &&
		project != nullptr && project == fActiveProject) {
		// TODO: if we are already building we should stop / relaunch build here.
		// at the moment we have an hack in place in ConsoleIOView::MessageReceived()
		// in the MSG_RUN_PROCESS case to handle this situation
		PostMessage(MSG_BUILD_PROJECT);
	}
}


void
GenioWindow::_FindGroupShow(bool show)
{
	_ShowView(fFindGroup, show, MSG_FIND_GROUP_TOGGLED);
	if (!show) {
		_ShowView(fReplaceGroup, show, MSG_REPLACE_GROUP_TOGGLED);
		Editor* editor = fTabManager->SelectedEditor();
		if (editor) {
			editor->GrabFocus();
		}
	} else {
		_GetFocusAndSelection(fFindTextControl);
	}
}


int32
GenioWindow::_FindMarkAll(const BString text)
{
	Editor* editor = fTabManager->SelectedEditor();
	if (!editor)
		return 0;

	int flags = editor->SetSearchFlags(fFindCaseSensitiveCheck->Value(),
										fFindWholeWordCheck->Value(),
										false, false, false);

	int countMarks = editor->FindMarkAll(text, flags);

	editor->GrabFocus();

	_UpdateFindMenuItems(text);

	return countMarks;
}


void
GenioWindow::_FindNext(const BString& strToFind, bool backwards)
{
	if (strToFind.IsEmpty())
		return;

	Editor* editor = fTabManager->SelectedEditor();

	if (!editor)
		return;

	editor->GrabFocus();

	int flags = editor->SetSearchFlags(fFindCaseSensitiveCheck->Value(),
										fFindWholeWordCheck->Value(),
										false, false, false);
	bool wrap = fFindWrapCheck->Value();

	if (backwards == false)
		editor->FindNext(strToFind, flags, wrap);
	else
		editor->FindPrevious(strToFind, flags, wrap);

	_UpdateFindMenuItems(strToFind);
}


void
GenioWindow::_FindInFiles()
{
	if (!fActiveProject)
		return;

	BString text(fFindTextControl->Text());
	if (text.IsEmpty())
		return;

	// convert checkboxes to grep parameters..
	BString extraParameters;
	if ((bool)fFindWholeWordCheck->Value())
		extraParameters += "w";

	if ((bool)fFindCaseSensitiveCheck->Value() == false)
		extraParameters += "i";

	text.CharacterEscape("\\\n\"", '\\');

	BString grepCommand("grep");
	BString excludeDir(gCFG["find_exclude_directory"]);
	if (!excludeDir.IsEmpty()) {
		if (excludeDir.FindFirst(",") >= 0)
			grepCommand << " --exclude-dir={" << excludeDir << "}";
		else
			grepCommand << " --exclude-dir=" << excludeDir << "";
	}

	grepCommand += " -IHrn";
	grepCommand += extraParameters;
	grepCommand += " -- ";
	grepCommand += EscapeQuotesWrap(text);
	grepCommand += " ";
	grepCommand += EscapeQuotesWrap(fActiveProject->Path());

	LogInfo("Find in file, executing: [%s]", grepCommand.String());
	fSearchResultPanel->StartSearch(grepCommand, fActiveProject->Path());

	_ShowLog(kSearchResult);
}


int32
GenioWindow::_GetEditorIndex(entry_ref* ref) const
{
	BEntry entry(ref, true);
	int32 filesCount = fTabManager->CountTabs();
	for (int32 index = 0; index < filesCount; index++) {
		Editor* editor = fTabManager->EditorAt(index);
		if (editor == nullptr) {
			BString notification;
			notification
				<< "Index " << index << ": NULL editor pointer";
			LogInfo(notification.String());
			continue;
		}
		BEntry matchEntry(editor->FileRef(), true);
		if (matchEntry == entry)
			return index;
	}
	return -1;
}


int32
GenioWindow::_GetEditorIndex(node_ref* nref) const
{
	int32 filesCount = fTabManager->CountTabs();
	for (int32 index = 0; index < filesCount; index++) {
		Editor* editor = fTabManager->EditorAt(index);
		if (editor == nullptr) {
			BString notification;
			notification
				<< "Index " << index << ": NULL editor pointer";
			LogInfo(notification.String());
			continue;
		}
		if (*nref == *editor->NodeRef())
			return index;
	}
	return -1;
}


void
GenioWindow::_GetFocusAndSelection(BTextControl* control) const
{
	control->MakeFocus(true);
	// If some text is selected, use that TODO index check
	Editor* editor = fTabManager->SelectedEditor();
	if (editor && editor->IsTextSelected()) {
		int32 size = editor->SendMessage(SCI_GETSELTEXT, 0, 0);
		char text[size + 1];
		editor->SendMessage(SCI_GETSELTEXT, 0, (sptr_t)text);
		control->SetText(text);
	} else
		control->TextView()->Clear();
}


status_t
GenioWindow::_Git(const BString& git_command)
{
	// Should not happen
	if (fActiveProject == nullptr)
		return B_ERROR;

	// Pretend building or running
	_UpdateProjectActivation(false);

	fConsoleIOView->Clear();
	_ShowLog(kOutputLog);

	BString command;
	command	<< "git " << git_command;

	BMessage message;
	message.AddString("cmd", command);
	message.AddString("cmd_type", command);

	// Go to appropriate directory
	chdir(fActiveProject->Path());

	return fConsoleIOView->RunCommand(&message);
}


void
GenioWindow::_HandleExternalMoveModification(entry_ref* oldRef, entry_ref* newRef)
{
	BEntry oldEntry(oldRef, true), newEntry(newRef, true);
	BPath oldPath, newPath;

	oldEntry.GetPath(&oldPath);
	newEntry.GetPath(&newPath);

	BString text;
	text << GenioNames::kApplicationName << ":\n";
	text << B_TRANSLATE("File \"%file%\" was apparently moved.\n"
		 "Do you want to ignore, close or reload it?");
	text.ReplaceAll("%file%", oldRef->name);

	BAlert* alert = new BAlert("FileMoveDialog", text,
 		B_TRANSLATE("Ignore"), B_TRANSLATE("Close"), B_TRANSLATE("Reload"),
 		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

 	alert->SetShortcut(0, B_ESCAPE);

	int32 index = _GetEditorIndex(oldRef);

 	int32 choice = alert->Go();

	if (choice == 0)
		return;
	else if (choice == 1) {
		_FileRequestClose(index);
	} else if (choice == 2) {
		Editor *editor = fTabManager->EditorAt(index);
		editor->SetFileRef(newRef);
		fTabManager->SetTabLabel(index, editor->Name().String());
		_UpdateLabel(index, editor->IsModified());

		// if the file is moved outside of the project folder it should be
		// detached from it as well
		auto project = editor->GetProjectFolder();
		if (project) {
			if (editor->FilePath().StartsWith(project->Path()) == false)
				editor->SetProjectFolder(NULL);
		} else {
			BString baseDir("");
			for (int32 index = 0; index < GetProjectBrowser()->CountProjects(); index++) {
				ProjectFolder * project = GetProjectBrowser()->ProjectAt(index);
				BString projectPath = project->Path();
				projectPath = projectPath.Append("/");
				if (editor->FilePath().StartsWith(projectPath)) {
					editor->SetProjectFolder(project);
				}
			}
		}

		BString notification;
		notification << "File info: " << oldPath.Path()
			<< " moved externally to " << newPath.Path();
		LogInfo(notification.String());
	}
}


void
GenioWindow::_HandleExternalRemoveModification(int32 index)
{
	if (index < 0) {
		return; //TODO notify
	}

	Editor* editor = fTabManager->EditorAt(index);
	BString fileName(editor->Name());

	BString text;
	text << GenioNames::kApplicationName << ":\n";
	text << B_TRANSLATE("File \"%file%\" was apparently removed.\n"
		"Do you want to keep the file or discard it?\n"
		"If kept and modified, save it or it will be lost.");

	text.ReplaceFirst("%file%", fileName);

	BAlert* alert = new BAlert("FileRemoveDialog", text,
 		B_TRANSLATE("Keep"), B_TRANSLATE("Discard"), nullptr,
 		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

 	alert->SetShortcut(0, B_ESCAPE);

 	int32 choice = alert->Go();

 	if (choice == 0) {
	 	// If not modified save it or it will be lost, if modified let
	 	// the user decide
	 	if (editor->IsModified() == false)
			_FileSave(index);
		return;
	} else if (choice == 1) {
		_RemoveTab(index);

		BString notification;
		notification << "File info: " << fileName << " removed externally";
		LogInfo(notification.String());
	}
}


void
GenioWindow::_HandleExternalStatModification(int32 index)
{
	if (index < 0)
		return;

	Editor* editor = fTabManager->EditorAt(index);

	BString text;
	text << GenioNames::kApplicationName << ":\n";
	text << (B_TRANSLATE("File \"%file%\" was apparently modified, reload it?"));
	text.ReplaceFirst("%file%", editor->Name());

	BAlert* alert = new BAlert("FileReloadDialog", text,
 		B_TRANSLATE("Ignore"), B_TRANSLATE("Reload"), nullptr,
 		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

 	alert->SetShortcut(0, B_ESCAPE);

	editor->StopMonitoring();
 	if (alert->Go() == 1) {
		editor->Reload();
		LogInfoF("File info: %s modified externally", editor->Name().String());
	}
	editor->StartMonitoring();
}


void
GenioWindow::_HandleNodeMonitorMsg(BMessage* msg)
{
	int32 opcode;
	status_t status;
	if ((status = msg->FindInt32("opcode", &opcode)) != B_OK) {
		// TODO notify
		return;
	}

	switch (opcode) {
		case B_ENTRY_MOVED: {
			int32 device;
			int64 srcDir;
			int64 dstDir;
			BString name;
			const char* oldName;

			if (msg->FindInt32("device", &device) != B_OK
				|| msg->FindInt64("to directory", &dstDir) != B_OK
				|| msg->FindInt64("from directory", &srcDir) != B_OK
				|| msg->FindString("name", &name) != B_OK
				|| msg->FindString("from name", &oldName) != B_OK)
					break;

			entry_ref oldRef(device, srcDir, oldName);
			entry_ref newRef(device, dstDir, name);

			_HandleExternalMoveModification(&oldRef, &newRef);

			break;
		}
		case B_ENTRY_REMOVED: {
			node_ref nref;
			BString name;
			int64 dir;

			if (msg->FindInt32("device", &nref.device) != B_OK
				|| msg->FindString("name", &name) != B_OK
				|| msg->FindInt64("directory", &dir) != B_OK
				|| msg->FindInt64("node", &nref.node) != B_OK)
					break;

			// Special case: not real B_ENTRY_REMOVED.
			// Happens on a 'git switch' command.
			node_ref dirRef;
			dirRef.device = nref.device;
			dirRef.node = dir;
			BDirectory fileDir(&dirRef);
			if (fileDir.InitCheck() == B_OK) {
				BEntry entry;
				if (fileDir.GetEntry(&entry) == B_OK) {
					BPath path;
					entry.GetPath(&path);
					if (path.Append(name.String()) == B_OK) {
						entry.SetTo(path.Path());
						if (entry.Exists()) {
						// this will automatically send a file update.
							return;
						}
					}

				}
			}

			_HandleExternalRemoveModification(_GetEditorIndex(&nref));
			break;
		}
		case B_STAT_CHANGED: {
			node_ref nref;
			int32 fields;

			if (msg->FindInt32("device", &nref.device) != B_OK
				|| msg->FindInt64("node", &nref.node) != B_OK
				|| msg->FindInt32("fields", &fields) != B_OK)
					break;
#if defined DEBUG
switch (fields) {
	case B_STAT_MODE:
	case B_STAT_UID:
	case B_STAT_GID:
		LogDebugF("MODES");
		break;
	case B_STAT_SIZE:
		LogDebugF("B_STAT_SIZE");
		break;
	case B_STAT_ACCESS_TIME:
		LogDebugF("B_STAT_ACCESS_TIME");
		break;
	case B_STAT_MODIFICATION_TIME:
		LogDebugF("B_STAT_MODIFICATION_TIME");
		break;
	case B_STAT_CREATION_TIME:
		LogDebugF("B_STAT_CREATION_TIME");
		break;
	case B_STAT_CHANGE_TIME:
		LogDebugF("B_STAT_CHANGE_TIME");
		break;
	case B_STAT_INTERIM_UPDATE:
		LogDebugF("B_STAT_INTERIM_UPDATE");
		break;
	default:
		LogDebugF("fields is: %d", fields);
		break;
}
#endif
			if ((fields & (B_STAT_MODE | B_STAT_UID | B_STAT_GID)) != 0)
				; 			// TODO recheck permissions
			/*
			 * Note: Pe and StyledEdit seems to cope differently on modifications,
			 *       firing different messages on the same modification of the
			 *       same file.
			 *   E.g. on changing file size
			 *				Pe fires				StyledEdit fires
			 *			B_STAT_CHANGE_TIME			B_STAT_CHANGE_TIME
			 *			B_STAT_CHANGE_TIME			fields is: 0x2008
			 *			fields is: 0x28				B_STAT_CHANGE_TIME
			 *										B_STAT_CHANGE_TIME
			 *										B_STAT_CHANGE_TIME
			 *										B_STAT_MODIFICATION_TIME
			 *
			 *   E.g. on changing file data but keeping the same file size
			 *				Pe fires				StyledEdit fires
			 *			B_STAT_CHANGE_TIME			B_STAT_CHANGE_TIME
			 *			B_STAT_CHANGE_TIME			fields is: 0x2008
			 *			B_STAT_MODIFICATION_TIME	B_STAT_CHANGE_TIME
			 *										B_STAT_CHANGE_TIME
			 *										B_STAT_CHANGE_TIME
			 *										B_STAT_MODIFICATION_TIME
			 */
			if (((fields & B_STAT_MODIFICATION_TIME)  != 0)
			// Do not reload if the file just got touched
				&& ((fields & B_STAT_ACCESS_TIME)  == 0)) {
				_HandleExternalStatModification(_GetEditorIndex(&nref));
			}

			break;
		}
		default:
			break;
	}
}


void
GenioWindow::_InitCentralSplit()
{
	// Find group
	fFindMenuField = new BMenuField("FindMenuField", NULL, new BMenu(B_TRANSLATE("Find:")));
	fFindMenuField->SetExplicitMaxSize(BSize(kFindReplaceOPSize, B_SIZE_UNSET));
	fFindMenuField->SetExplicitMinSize(BSize(kFindReplaceOPSize, B_SIZE_UNSET));

	fFindTextControl = new BTextControl("FindTextControl", "" , "", nullptr);
	fFindTextControl->TextView()->SetMaxBytes(kFindReplaceMaxBytes);
	float charWidth = fFindTextControl->StringWidth("0", 1);
	fFindTextControl->SetExplicitMinSize(
						BSize(charWidth * kFindReplaceMinBytes + 10.0f,
						B_SIZE_UNSET));
	fFindTextControl->SetExplicitMaxSize(fFindTextControl->MinSize());


	fFindCaseSensitiveCheck = new BCheckBox(B_TRANSLATE("Match case"), new BMessage(MSG_FIND_MATCH_CASE));
	fFindWholeWordCheck = new BCheckBox(B_TRANSLATE("Whole word"), new BMessage(MSG_FIND_WHOLE_WORD));
	fFindWrapCheck = new BCheckBox(B_TRANSLATE("Wrap"), new BMessage(MSG_FIND_WRAP));

	fFindWrapCheck->SetValue(gCFG["find_wrap"] ? B_CONTROL_ON : B_CONTROL_OFF);
	fFindWholeWordCheck->SetValue(gCFG["find_whole_word"] ? B_CONTROL_ON : B_CONTROL_OFF);
	fFindCaseSensitiveCheck->SetValue(gCFG["find_match_case"] ? B_CONTROL_ON : B_CONTROL_OFF);

	fFindGroup = new ToolBar(this);
	fFindGroup->ChangeIconSize(kDefaultIconSize);
	fFindGroup->AddView(BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
												.Add(fFindMenuField)
												.Add(fFindTextControl).View());

	ActionManager::AddItem(MSG_FIND_NEXT, fFindGroup);
	ActionManager::AddItem(MSG_FIND_PREVIOUS, fFindGroup);
	fFindGroup->AddView(BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
												.Add(fFindWrapCheck)
												.Add(fFindWholeWordCheck)
												.Add(fFindCaseSensitiveCheck).View());

	ActionManager::AddItem(MSG_FIND_MARK_ALL, fFindGroup);
	ActionManager::AddItem(MSG_FIND_IN_FILES, fFindGroup);
	fFindGroup->AddGlue();

	fFindGroup->Hide();

	// Replace group
	fReplaceMenuField = new BMenuField("ReplaceMenu", NULL,
										new BMenu(B_TRANSLATE("Replace:")));
	fReplaceMenuField->SetExplicitMaxSize(BSize(kFindReplaceOPSize, B_SIZE_UNSET));
	fReplaceMenuField->SetExplicitMinSize(BSize(kFindReplaceOPSize, B_SIZE_UNSET));

	fReplaceTextControl = new BTextControl("ReplaceTextControl", "", "", NULL);
	fReplaceTextControl->TextView()->SetMaxBytes(kFindReplaceMaxBytes);
	fReplaceTextControl->SetExplicitMaxSize(fFindTextControl->MaxSize());
	fReplaceTextControl->SetExplicitMinSize(fFindTextControl->MinSize());

	fReplaceGroup = new ToolBar(this);
	fReplaceGroup->ChangeIconSize(kDefaultIconSize);
	fReplaceGroup->AddView(BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
												.Add(fReplaceMenuField)
												.Add(fReplaceTextControl).View());
	fReplaceGroup->AddAction(MSG_REPLACE_ONE, B_TRANSLATE("Replace selection"), "kIconReplaceOne");
	fReplaceGroup->AddAction(MSG_REPLACE_NEXT, B_TRANSLATE("Replace and find next"), "kIconReplaceNext");
	fReplaceGroup->AddAction(MSG_REPLACE_PREVIOUS, B_TRANSLATE("Replace and find previous"), "kIconReplacePrev");
	fReplaceGroup->AddAction(MSG_REPLACE_ALL, B_TRANSLATE("Replace all"), "kIconReplaceAll");
	fReplaceGroup->AddGlue();
	fReplaceGroup->Hide();


	// Run group
	fRunConsoleProgramText = new BTextControl("ReplaceTextControl", "", "",
		new BMessage(MSG_RUN_CONSOLE_PROGRAM));
	fRunConsoleProgramButton = new BButton("RunConsoleProgramButton",
		B_TRANSLATE("Run"), new BMessage(MSG_RUN_CONSOLE_PROGRAM));

	BString tooltip("cwd: ");
	tooltip << (const char*)gCFG["projects_directory"];
	fRunConsoleProgramText->SetToolTip(tooltip);

	fRunConsoleProgramGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0f)
		.Add(BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fRunConsoleProgramText)
			.Add(fRunConsoleProgramButton)
			.AddGlue()
		)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
	;
	fRunConsoleProgramGroup->SetVisible(false);

	// Editor tab & view

	fTabManager = new EditorTabManager(BMessenger(this));
	fTabManager->GetTabContainerView()->SetExplicitMaxSize(BSize(B_SIZE_UNSET, fProjectsTabView->TabHeight()));
	fTabManager->GetTabContainerView()->SetExplicitMinSize(BSize(B_SIZE_UNSET, fProjectsTabView->TabHeight()));

	dirtyFrameHack = fTabManager->TabGroup()->Frame();

	fEditorTabsGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0f)
		.Add(fRunConsoleProgramGroup)
		.Add(fFindGroup)
		.Add(fReplaceGroup)
		.Add(fTabManager->TabGroup())
		.Add(fTabManager->ContainerView())
	;

	fFindWrapCheck->SetTarget(this);
	fFindWholeWordCheck->SetTarget(this);
	fFindCaseSensitiveCheck->SetTarget(this);
}


void
GenioWindow::_ShowView(BView* view, bool show, int32 msgWhat)
{
	if (show && view->IsHidden())
		view->Show();
	if (!show && !view->IsHidden())
		view->Hide();

	if (msgWhat > -1)
		ActionManager::SetPressed(msgWhat, !view->IsHidden());
}


void
GenioWindow::_InitActions()
{
	ActionManager::RegisterAction(MSG_FILE_OPEN,
								   B_TRANSLATE("Open" B_UTF8_ELLIPSIS),
								   "", "", 'O');

	ActionManager::RegisterAction(MSG_FILE_NEW,
								   B_TRANSLATE("New"),
								   "", "");

	ActionManager::RegisterAction(MSG_FILE_SAVE,
								   B_TRANSLATE("Save"),
								   B_TRANSLATE("Save current file"),
								   "kIconSave", 'S');

	ActionManager::RegisterAction(MSG_FILE_SAVE_AS,
								   B_TRANSLATE("Save as" B_UTF8_ELLIPSIS),
								   "","");

	ActionManager::RegisterAction(MSG_FILE_SAVE_ALL,
								   B_TRANSLATE("Save all"),
								   B_TRANSLATE("Save all files"),
								   "kIconSaveAll", 'S', B_SHIFT_KEY);


	ActionManager::RegisterAction(MSG_FILE_CLOSE,
								   B_TRANSLATE("Close"),
								   B_TRANSLATE("Close file"),
								   "kIconClose", 'W');

	ActionManager::RegisterAction(MSG_FILE_CLOSE_ALL,
								   B_TRANSLATE("Close all"),
								   "", "", 'W', B_SHIFT_KEY);

	ActionManager::RegisterAction(B_QUIT_REQUESTED,
	                               B_TRANSLATE("Quit"),
								   "", "", 'Q');
	ActionManager::RegisterAction(B_UNDO,
								   B_TRANSLATE("Undo"),
								   B_TRANSLATE("Undo"),
								   "kIconUndo", 'Z');
	ActionManager::RegisterAction(B_REDO,
								   B_TRANSLATE("Redo"),
								   B_TRANSLATE("Redo"),
								   "kIconRedo", 'Z', B_SHIFT_KEY);

	ActionManager::RegisterAction(B_CUT,
	                               B_TRANSLATE("Cut"),
								   "", "", 'X');
	ActionManager::RegisterAction(B_COPY,
	                               B_TRANSLATE("Copy"),
								   "", "", 'C');
	ActionManager::RegisterAction(B_PASTE,
	                               B_TRANSLATE("Paste"),
								   "", "", 'V');
	ActionManager::RegisterAction(B_SELECT_ALL,
								   B_TRANSLATE("Select all"),
								   "", "", 'A');
	ActionManager::RegisterAction(MSG_TEXT_OVERWRITE,
								   B_TRANSLATE("Overwrite"),
								   "", "", B_INSERT);
	ActionManager::RegisterAction(MSG_FILE_FOLD_TOGGLE,
								   B_TRANSLATE("Fold/Unfold all"),
								   B_TRANSLATE("Fold/Unfold all"),
								   "kIconFold_4");
	ActionManager::RegisterAction(MSG_WHITE_SPACES_TOGGLE,
								   B_TRANSLATE("Show white spaces"),
								   B_TRANSLATE("Show white spaces"), "kIconShowPunctuation");
	ActionManager::RegisterAction(MSG_LINE_ENDINGS_TOGGLE,
								   B_TRANSLATE("Show line endings"),
								   B_TRANSLATE("Show line endings"), "");

	ActionManager::RegisterAction(MSG_FILE_TRIM_TRAILING_SPACE,
								  B_TRANSLATE("Trim trailing whitespace"),
								  "", "");


	ActionManager::RegisterAction(MSG_DUPLICATE_LINE,
								   B_TRANSLATE("Duplicate current line"),
								   "", "", 'K');
	ActionManager::RegisterAction(MSG_DELETE_LINES,
								   B_TRANSLATE("Delete current line"),
								   "", "", 'D');

	ActionManager::RegisterAction(MSG_COMMENT_SELECTED_LINES,
								   B_TRANSLATE("Comment selected lines"),
								   "", "", 'C', B_SHIFT_KEY);

	ActionManager::RegisterAction(MSG_AUTOCOMPLETION,
								   B_TRANSLATE("Autocomplete"), "","", B_SPACE);

	ActionManager::RegisterAction(MSG_FORMAT,
								   B_TRANSLATE("Format"));

	ActionManager::RegisterAction(MSG_GOTODEFINITION,
								   B_TRANSLATE("Go to definition"), "","", 'G');

	ActionManager::RegisterAction(MSG_GOTODECLARATION,
								   B_TRANSLATE("Go to declaration"));

	ActionManager::RegisterAction(MSG_GOTOIMPLEMENTATION,
								   B_TRANSLATE("Go to implementation"));

	ActionManager::RegisterAction(MSG_SWITCHSOURCE,
								   B_TRANSLATE("Switch source/header"), "", "", B_TAB);

	ActionManager::RegisterAction(MSG_VIEW_ZOOMIN,  B_TRANSLATE("Zoom in"), "", "", '+');
	ActionManager::RegisterAction(MSG_VIEW_ZOOMOUT, B_TRANSLATE("Zoom out"), "", "", '-');
	ActionManager::RegisterAction(MSG_VIEW_ZOOMRESET, B_TRANSLATE("Reset zoom"), "", "", '0');


	ActionManager::RegisterAction(MSG_FIND_GROUP_TOGGLED, //MSG_FIND_GROUP_SHOW,
								   B_TRANSLATE("Find"),
								   B_TRANSLATE("Show/Hide find bar"),
								   "kIconFind");

	ActionManager::RegisterAction(MSG_REPLACE_GROUP_TOGGLED, //MSG_REPLACE_GROUP_SHOW,
								   B_TRANSLATE("Replace"),
								   B_TRANSLATE("Show/Hide replace bar"),
								   "kIconReplace");


	ActionManager::RegisterAction(MSG_FIND_GROUP_SHOW,
								   B_TRANSLATE("Find"),
								   "", "", 'F');

	ActionManager::RegisterAction(MSG_REPLACE_GROUP_SHOW,
								   B_TRANSLATE("Replace"),
								   "", "", 'R');

	ActionManager::RegisterAction(MSG_GOTO_LINE,
								   B_TRANSLATE("Go to line" B_UTF8_ELLIPSIS),
								   "", "", ',');

	ActionManager::RegisterAction(MSG_PROJECT_OPEN,
								   B_TRANSLATE("Open project" B_UTF8_ELLIPSIS),
								   "","",'O', B_OPTION_KEY);

	ActionManager::RegisterAction(MSG_PROJECT_OPEN_REMOTE,
								   B_TRANSLATE("Open remote project" B_UTF8_ELLIPSIS),
								   "","",'O', B_SHIFT_KEY | B_OPTION_KEY);

	ActionManager::RegisterAction(MSG_PROJECT_CLOSE,
								   B_TRANSLATE("Close project"),
								   "", "", 'C', B_OPTION_KEY);

	ActionManager::RegisterAction(MSG_RUN_CONSOLE_PROGRAM_SHOW,
								   B_TRANSLATE("Run console program"),
								   B_TRANSLATE("Run console program"),
								   "kIconTerminal");
//add missing menus

	ActionManager::RegisterAction(MSG_SHOW_HIDE_PROJECTS,
								   B_TRANSLATE("Show projects pane"),
								   B_TRANSLATE("Show/Hide projects pane"),
								   "kIconWinNav");

	ActionManager::RegisterAction(MSG_SHOW_HIDE_OUTPUT,
								   B_TRANSLATE("Show output pane"),
	                               B_TRANSLATE("Show/Hide output pane"),
								   "kIconWinStat");

	ActionManager::RegisterAction(MSG_FULLSCREEN,
								   B_TRANSLATE("Fullscreen"),
	                               B_TRANSLATE("Fullscreen"),
								   "", B_ENTER);

	ActionManager::RegisterAction(MSG_FOCUS_MODE,
								   B_TRANSLATE("Focus mode"),
	                               B_TRANSLATE("Focus mode"),
								   "", B_ENTER, B_SHIFT_KEY);

	ActionManager::RegisterAction(MSG_TOGGLE_TOOLBAR,
								   B_TRANSLATE("Show toolbar"));


	ActionManager::RegisterAction(MSG_BUILD_PROJECT,
								  B_TRANSLATE("Build project"),
								  B_TRANSLATE("Build project"),
								  "kIconBuild", 'B');

	ActionManager::RegisterAction(MSG_CLEAN_PROJECT,
								  B_TRANSLATE("Clean project"),
								  B_TRANSLATE("Clean project"),
								  "kIconClean");

	ActionManager::RegisterAction(MSG_RUN_TARGET,
								  B_TRANSLATE("Run target"),
								  B_TRANSLATE("Run target"),
								  "kIconRun", 'R', B_SHIFT_KEY);

	ActionManager::RegisterAction(MSG_DEBUG_PROJECT,
								  B_TRANSLATE("Debug project"),
								  B_TRANSLATE("Debug project"),
								  "kIconDebug");

	ActionManager::RegisterAction(MSG_PROJECT_SETTINGS,
								  B_TRANSLATE("Project settings" B_UTF8_ELLIPSIS),
								  B_TRANSLATE("Project settings" B_UTF8_ELLIPSIS),
								  "");

	ActionManager::RegisterAction(MSG_BUFFER_LOCK,
								  B_TRANSLATE("Read-only"),
								  B_TRANSLATE("Make file read-only"), "kIconUnlocked");

	ActionManager::RegisterAction(MSG_FILE_PREVIOUS_SELECTED, "",
						          B_TRANSLATE("Switch to previous file"), "kIconBack_1");

	ActionManager::RegisterAction(MSG_FILE_NEXT_SELECTED, "",
								  B_TRANSLATE("Switch to next file"), "kIconForward_2");

	// Find Panel
	ActionManager::RegisterAction(MSG_FIND_NEXT,
								  B_TRANSLATE("Find next"),
								  B_TRANSLATE("Find next"),
								   "kIconDown_3",
								  B_DOWN_ARROW, B_COMMAND_KEY);

	ActionManager::RegisterAction(MSG_FIND_PREVIOUS,
								  B_TRANSLATE("Find previous"),
								  B_TRANSLATE("Find previous"),
								  "kIconUp_3",
								  B_UP_ARROW, B_COMMAND_KEY);
	ActionManager::RegisterAction(MSG_FIND_IN_FILES,
								  B_TRANSLATE("Find in project"),
								  B_TRANSLATE("Find in project"),
								  "kIconFindInFiles");

	ActionManager::RegisterAction(MSG_FIND_MARK_ALL,
								  B_TRANSLATE("Bookmark all"),
								  B_TRANSLATE("Bookmark all"),
								  "kIconBookmarkPen");

}


void
GenioWindow::_InitMenu()
{
	// Menu
	fMenuBar = new BMenuBar("menubar");

	BMenu* appMenu = new BMenu("");
	appMenu->AddItem(new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED)));
	appMenu->AddItem(new BMenuItem(B_TRANSLATE("Genio project" B_UTF8_ELLIPSIS),
		new BMessage(MSG_HELP_GITHUB)));
	appMenu->AddSeparatorItem();
	appMenu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(MSG_WINDOW_SETTINGS), 'P', B_OPTION_KEY));
	appMenu->AddSeparatorItem();
	ActionManager::AddItem(B_QUIT_REQUESTED, appMenu);

	IconMenuItem* iconMenu = nullptr;

	//xmas-icon!
	if (IsXMasPeriod()) {
		BBitmap* iconXMAS = new BBitmap(BRect(BPoint(0, 0), be_control_look->ComposeIconSize(B_MINI_ICON)), B_RGBA32);
		GetVectorIcon("xmas-icon", iconXMAS);
		iconMenu = new IconMenuItem(appMenu, nullptr, iconXMAS, B_MINI_ICON);
	} else {
		iconMenu = new IconMenuItem(appMenu, nullptr, GenioNames::kApplicationSignature, B_MINI_ICON);
	}
	fMenuBar->AddItem(iconMenu);

	BMenu* fileMenu = new BMenu(B_TRANSLATE("File"));

	//ActionManager::AddItem(MSG_FILE_NEW,      fileMenu);

	fileMenu->AddItem(fFileNewMenuItem = new TemplatesMenu(this, B_TRANSLATE("New"),
			new BMessage(MSG_FILE_NEW), new BMessage(MSG_SHOW_TEMPLATE_USER_FOLDER),
			TemplateManager::GetDefaultTemplateDirectory(),
			TemplateManager::GetUserTemplateDirectory(),
			TemplatesMenu::SHOW_ALL_VIEW_MODE,	true));

	ActionManager::AddItem(MSG_FILE_OPEN,     fileMenu);

	fileMenu->AddItem(new BMenuItem(BRecentFilesList::NewFileListMenu(
			B_TRANSLATE("Open recent" B_UTF8_ELLIPSIS), nullptr, nullptr, this,
			kRecentFilesNumber, true, nullptr, GenioNames::kApplicationSignature), nullptr));

	fileMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_FILE_SAVE,     fileMenu);
	ActionManager::AddItem(MSG_FILE_SAVE_AS,  fileMenu);
	ActionManager::AddItem(MSG_FILE_SAVE_ALL, fileMenu);

	fileMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_FILE_CLOSE,     fileMenu);
	ActionManager::AddItem(MSG_FILE_CLOSE_ALL, fileMenu);

	ActionManager::SetEnabled(MSG_FILE_NEW,  false);
	ActionManager::SetEnabled(MSG_FILE_SAVE, false);
	ActionManager::SetEnabled(MSG_FILE_SAVE_AS, false);
	ActionManager::SetEnabled(MSG_FILE_SAVE_ALL, false);
	ActionManager::SetEnabled(MSG_FILE_CLOSE, false);
	ActionManager::SetEnabled(MSG_FILE_CLOSE_ALL, false);

	fMenuBar->AddItem(fileMenu);

	BMenu* editMenu = new BMenu(B_TRANSLATE("Edit"));

	ActionManager::AddItem(B_UNDO, editMenu);
	ActionManager::AddItem(B_REDO, editMenu);

	editMenu->AddSeparatorItem();

	ActionManager::AddItem(B_CUT, editMenu);
	ActionManager::AddItem(B_COPY, editMenu);
	ActionManager::AddItem(B_PASTE, editMenu);

	editMenu->AddSeparatorItem();

	ActionManager::AddItem(B_SELECT_ALL, editMenu);

	editMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_TEXT_OVERWRITE, editMenu);

	editMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_DUPLICATE_LINE, editMenu);
	ActionManager::AddItem(MSG_DELETE_LINES, editMenu);
	ActionManager::AddItem(MSG_COMMENT_SELECTED_LINES, editMenu);
	ActionManager::AddItem(MSG_FILE_TRIM_TRAILING_SPACE, editMenu);

	editMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_AUTOCOMPLETION, editMenu);
	ActionManager::AddItem(MSG_FORMAT, editMenu);

	editMenu->AddSeparatorItem();

	fLineEndingsMenu = new BMenu(B_TRANSLATE("Line endings"));
	fLineEndingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Unix"),
		new BMessage(MSG_EOL_CONVERT_TO_UNIX)));
	fLineEndingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Dos"),
		new BMessage(MSG_EOL_CONVERT_TO_DOS)));
	fLineEndingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Mac"),
		new BMessage(MSG_EOL_CONVERT_TO_MAC)));

	ActionManager::SetEnabled(B_UNDO, false);
	ActionManager::SetEnabled(B_REDO, false);

	ActionManager::SetEnabled(B_CUT, false);
	ActionManager::SetEnabled(B_COPY, false);
	ActionManager::SetEnabled(B_PASTE, false);
	ActionManager::SetEnabled(B_SELECT_ALL, false);
	ActionManager::SetEnabled(MSG_TEXT_OVERWRITE, false);
	ActionManager::SetEnabled(MSG_DUPLICATE_LINE, false);
	ActionManager::SetEnabled(MSG_DELETE_LINES, false);
	ActionManager::SetEnabled(MSG_COMMENT_SELECTED_LINES, false);
	ActionManager::SetEnabled(MSG_FILE_TRIM_TRAILING_SPACE, false);

	ActionManager::SetEnabled(MSG_AUTOCOMPLETION, false);
	ActionManager::SetEnabled(MSG_FORMAT, false);

	fLineEndingsMenu->SetEnabled(false);

	editMenu->AddItem(fLineEndingsMenu);
	editMenu->AddItem(_CreateLanguagesMenu());

	fMenuBar->AddItem(editMenu);

	BMenu* viewMenu = new BMenu(B_TRANSLATE("View"));
	fMenuBar->AddItem(viewMenu);

	ActionManager::AddItem(MSG_VIEW_ZOOMIN, viewMenu);
	ActionManager::AddItem(MSG_VIEW_ZOOMOUT, viewMenu);
	ActionManager::AddItem(MSG_VIEW_ZOOMRESET, viewMenu);

	viewMenu->AddSeparatorItem();
	ActionManager::AddItem(MSG_FILE_FOLD_TOGGLE, viewMenu);
	ActionManager::AddItem(MSG_WHITE_SPACES_TOGGLE, viewMenu);
	ActionManager::AddItem(MSG_LINE_ENDINGS_TOGGLE, viewMenu);
	ActionManager::AddItem(MSG_SWITCHSOURCE, viewMenu);
	ActionManager::SetEnabled(MSG_FILE_FOLD_TOGGLE, false);
	ActionManager::SetEnabled(MSG_WHITE_SPACES_TOGGLE, false);
	ActionManager::SetEnabled(MSG_LINE_ENDINGS_TOGGLE, false);
	ActionManager::SetEnabled(MSG_SWITCHSOURCE, false);

	BMenu* searchMenu = new BMenu(B_TRANSLATE("Search"));
	ActionManager::AddItem(MSG_FIND_GROUP_SHOW, searchMenu);
	ActionManager::AddItem(MSG_REPLACE_GROUP_SHOW, searchMenu);
	ActionManager::AddItem(MSG_FIND_NEXT, searchMenu);
	ActionManager::AddItem(MSG_FIND_PREVIOUS, searchMenu);
	searchMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_GOTO_LINE, searchMenu);

	ActionManager::SetEnabled(MSG_GOTO_LINE, false);

	fBookmarksMenu = new BMenu(B_TRANSLATE("Bookmark"));
	fBookmarksMenu->AddItem(fBookmarkToggleItem = new BMenuItem(B_TRANSLATE_COMMENT("Toggle",
		"Bookmark menu"), new BMessage(MSG_BOOKMARK_TOGGLE)));
	fBookmarksMenu->AddItem(fBookmarkClearAllItem = new BMenuItem(B_TRANSLATE_COMMENT("Clear all",
		"Bookmark menu"), new BMessage(MSG_BOOKMARK_CLEAR_ALL)));
	fBookmarksMenu->AddItem(fBookmarkGoToNextItem = new BMenuItem(B_TRANSLATE_COMMENT("Next",
		"Bookmark menu"), new BMessage(MSG_BOOKMARK_GOTO_NEXT), 'N', B_CONTROL_KEY));
	fBookmarksMenu->AddItem(fBookmarkGoToPreviousItem = new BMenuItem(B_TRANSLATE_COMMENT(
		"Previous", "Bookmark menu"),
		new BMessage(MSG_BOOKMARK_GOTO_PREVIOUS),'P', B_CONTROL_KEY));

	fBookmarksMenu->SetEnabled(false);

	searchMenu->AddItem(fBookmarksMenu);

	ActionManager::AddItem(MSG_GOTODEFINITION, searchMenu);
	ActionManager::AddItem(MSG_GOTODECLARATION, searchMenu);
	ActionManager::AddItem(MSG_GOTOIMPLEMENTATION, searchMenu);

	ActionManager::SetEnabled(MSG_GOTODEFINITION, false);
	ActionManager::SetEnabled(MSG_GOTODECLARATION, false);
	ActionManager::SetEnabled(MSG_GOTOIMPLEMENTATION, false);

	fMenuBar->AddItem(searchMenu);

	BMenu* projectMenu = new BMenu(B_TRANSLATE("Project"));

	ActionManager::AddItem(MSG_PROJECT_OPEN, projectMenu);
	ActionManager::AddItem(MSG_PROJECT_OPEN_REMOTE, projectMenu);

	BMessage *openProjectFolderMessage = new BMessage(MSG_PROJECT_FOLDER_OPEN);
	projectMenu->AddItem(new BMenuItem(BRecentFoldersList::NewFolderListMenu(
			B_TRANSLATE("Open recent" B_UTF8_ELLIPSIS), openProjectFolderMessage, this,
			kRecentFilesNumber, false, GenioNames::kApplicationSignature), nullptr));

	ActionManager::AddItem(MSG_PROJECT_CLOSE, projectMenu);

	projectMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_BUILD_PROJECT, projectMenu);
	ActionManager::AddItem(MSG_CLEAN_PROJECT, projectMenu);
	ActionManager::AddItem(MSG_RUN_TARGET, projectMenu);

	projectMenu->AddSeparatorItem();

	fBuildModeItem = new BMenu(B_TRANSLATE("Build mode"));
	fBuildModeItem->SetRadioMode(true);
	fBuildModeItem->AddItem(fReleaseModeItem = new BMenuItem(B_TRANSLATE("Release"),
		new BMessage(MSG_BUILD_MODE_RELEASE)));
	fBuildModeItem->AddItem(fDebugModeItem = new BMenuItem(B_TRANSLATE("Debug"),
		new BMessage(MSG_BUILD_MODE_DEBUG)));
	fDebugModeItem->SetMarked(true);
	projectMenu->AddItem(fBuildModeItem);
	projectMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_DEBUG_PROJECT, projectMenu);

	projectMenu->AddSeparatorItem();
	projectMenu->AddItem(fMakeCatkeysItem = new BMenuItem (B_TRANSLATE("Make catkeys"),
		new BMessage(MSG_MAKE_CATKEYS)));
	projectMenu->AddItem(fMakeBindcatalogsItem = new BMenuItem (B_TRANSLATE("Make bindcatalogs"),
		new BMessage(MSG_MAKE_BINDCATALOGS)));

	ActionManager::SetEnabled(MSG_PROJECT_CLOSE, false);

	ActionManager::SetEnabled(MSG_BUILD_PROJECT, false);
	ActionManager::SetEnabled(MSG_CLEAN_PROJECT, false);
	ActionManager::SetEnabled(MSG_RUN_TARGET, false);

	fBuildModeItem->SetEnabled(false);
	ActionManager::SetEnabled(MSG_DEBUG_PROJECT, false);
	fMakeCatkeysItem->SetEnabled(false);
	fMakeBindcatalogsItem->SetEnabled(false);

	projectMenu->AddSeparatorItem();
	ActionManager::AddItem(MSG_PROJECT_SETTINGS, projectMenu);

	ActionManager::SetEnabled(MSG_PROJECT_SETTINGS, false);

	fMenuBar->AddItem(projectMenu);

	fGitMenu = new BMenu(B_TRANSLATE("Git"));

	fGitMenu->AddItem(fGitBranchItem = new BMenuItem(B_TRANSLATE_COMMENT("Branch",
		"The git command"), nullptr));
	BMessage* git_branch_message = new BMessage(MSG_GIT_COMMAND);
	git_branch_message->AddString("command", "branch");
	fGitBranchItem->SetMessage(git_branch_message);

	fGitMenu->AddItem(new SwitchBranchMenu(this, B_TRANSLATE("Switch to branch"),
											new BMessage(MSG_GIT_SWITCH_BRANCH)));

	fGitMenu->AddItem(fGitLogItem = new BMenuItem(B_TRANSLATE_COMMENT("Log",
		"The git command"), nullptr));
	BMessage* git_log_message = new BMessage(MSG_GIT_COMMAND);
	git_log_message->AddString("command", "log");
	fGitLogItem->SetMessage(git_log_message);

	fGitMenu->AddItem(fGitPullItem = new BMenuItem(B_TRANSLATE_COMMENT("Pull",
		"The git command"), nullptr));
	BMessage* git_pull_message = new BMessage(MSG_GIT_COMMAND);
	git_pull_message->AddString("command", "pull");
	fGitPullItem->SetMessage(git_pull_message);

	fGitMenu->AddItem(fGitStatusItem = new BMenuItem(B_TRANSLATE_COMMENT("Status",
		"The git command"), nullptr));
	BMessage* git_status_message = new BMessage(MSG_GIT_COMMAND);
	git_status_message->AddString("command", "status");
	fGitStatusItem->SetMessage(git_status_message);

	fGitMenu->AddItem(fGitShowConfigItem = new BMenuItem(B_TRANSLATE_COMMENT("Show config",
		"The git command"), nullptr));
	BMessage* git_config_message = new BMessage(MSG_GIT_COMMAND);
	git_config_message->AddString("command", "config --list");
	fGitShowConfigItem->SetMessage(git_config_message);

	fGitMenu->AddItem(fGitTagItem = new BMenuItem(B_TRANSLATE_COMMENT("Tag",
		"The git command"), nullptr));
	BMessage* git_tag_message = new BMessage(MSG_GIT_COMMAND);
	git_tag_message->AddString("command", "tag");
	fGitTagItem->SetMessage(git_tag_message);

	fGitMenu->AddSeparatorItem();

	fGitMenu->AddItem(fGitLogOnelineItem = new BMenuItem(B_TRANSLATE_COMMENT("Log (oneline)",
		"The git command"), nullptr));
	BMessage* git_log_oneline_message = new BMessage(MSG_GIT_COMMAND);
	git_log_oneline_message->AddString("command", "log --oneline --decorate");
	fGitLogOnelineItem->SetMessage(git_log_oneline_message);

	fGitMenu->AddItem(fGitPullRebaseItem = new BMenuItem(B_TRANSLATE_COMMENT("Pull (rebase)",
		"The git command"), nullptr));
	BMessage* git_pull_rebase_message = new BMessage(MSG_GIT_COMMAND);
	git_pull_rebase_message->AddString("command", "pull --rebase");
	fGitPullRebaseItem->SetMessage(git_pull_rebase_message);

	fGitMenu->AddItem(fGitStatusShortItem = new BMenuItem(B_TRANSLATE_COMMENT("Status (short)",
		"The git command"), nullptr));
	BMessage* git_status_short_message = new BMessage(MSG_GIT_COMMAND);
	git_status_short_message->AddString("command", "status --short");
	fGitStatusShortItem->SetMessage(git_status_short_message);

	fGitMenu->SetEnabled(false);
	fMenuBar->AddItem(fGitMenu);

	BMenu* windowMenu = new BMenu(B_TRANSLATE("Window"));

	BMenu* submenu = new BMenu(B_TRANSLATE("Appearance"));
	ActionManager::AddItem(MSG_SHOW_HIDE_PROJECTS, submenu);
	ActionManager::AddItem(MSG_SHOW_HIDE_OUTPUT,   submenu);
	ActionManager::AddItem(MSG_TOGGLE_TOOLBAR, submenu);
	windowMenu->AddItem(submenu);
	windowMenu->AddSeparatorItem();
	ActionManager::AddItem(MSG_FULLSCREEN, windowMenu);
	ActionManager::AddItem(MSG_FOCUS_MODE, windowMenu);
	fMenuBar->AddItem(windowMenu);
}


BMenu*
GenioWindow::_CreateLanguagesMenu()
{
	fLanguageMenu = new BMenu(B_TRANSLATE("Language"));

	for(size_t i = 0; i < Languages::GetCount(); ++i) {
		std::string lang = Languages::GetLanguage(i);
		std::string name = Languages::GetMenuItemName(lang);
		fLanguageMenu->AddItem(new BMenuItem(name.c_str(), SMSG(MSG_SET_LANGUAGE, {"lang", lang.c_str()})));
	}
	fLanguageMenu->SetRadioMode(true);
	fLanguageMenu->SetEnabled(false);
	return fLanguageMenu;
}


void
GenioWindow::_InitToolbar()
{
	fToolBar = new ToolBar(this);
	fToolBar->ChangeIconSize(kDefaultIconSize);

	ActionManager::AddItem(MSG_SHOW_HIDE_PROJECTS, fToolBar);
	ActionManager::AddItem(MSG_SHOW_HIDE_OUTPUT,   fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_FILE_FOLD_TOGGLE, fToolBar);
	ActionManager::AddItem(B_UNDO, fToolBar);
	ActionManager::AddItem(B_REDO, fToolBar);
	ActionManager::AddItem(MSG_FILE_SAVE, fToolBar);
	ActionManager::AddItem(MSG_FILE_SAVE_ALL, fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_WHITE_SPACES_TOGGLE, fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_BUILD_PROJECT, fToolBar);
	ActionManager::AddItem(MSG_CLEAN_PROJECT, fToolBar);
	ActionManager::AddItem(MSG_RUN_TARGET, fToolBar);
	ActionManager::AddItem(MSG_DEBUG_PROJECT, fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_FIND_GROUP_TOGGLED,		fToolBar);
	ActionManager::AddItem(MSG_REPLACE_GROUP_TOGGLED,	fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_RUN_CONSOLE_PROGRAM_SHOW, fToolBar);
	fToolBar->AddGlue();

	ActionManager::AddItem(MSG_BUFFER_LOCK, fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_FILE_PREVIOUS_SELECTED, fToolBar);
	ActionManager::AddItem(MSG_FILE_NEXT_SELECTED, fToolBar);
	ActionManager::AddItem(MSG_FILE_CLOSE, fToolBar);

	fToolBar->AddAction(MSG_FILE_MENU_SHOW, B_TRANSLATE("Open files list"), "kIconFileList");

	ActionManager::SetEnabled(MSG_FIND_GROUP_TOGGLED, true);
	ActionManager::SetEnabled(MSG_REPLACE_GROUP_TOGGLED, true);
}


void
GenioWindow::_InitOutputSplit()
{
	// Output
	fOutputTabView = new BTabView("OutputTabview");

	fProblemsPanel = new ProblemsPanel(fOutputTabView);

	fBuildLogView = new ConsoleIOView(B_TRANSLATE("Build log"), BMessenger(this));

	fConsoleIOView = new ConsoleIOView(B_TRANSLATE("Console I/O"), BMessenger(this));

	fSearchResultPanel = new SearchResultPanel(fOutputTabView);

	fOutputTabView->AddTab(fProblemsPanel);
	fOutputTabView->AddTab(fBuildLogView);
	fOutputTabView->AddTab(fConsoleIOView);
	fOutputTabView->AddTab(fSearchResultPanel);
}


void
GenioWindow::_InitSideSplit()
{
	// Projects View
	fProjectsTabView = new BTabView("ProjectsTabview");

	fProjectsFolderBrowser = new ProjectsFolderBrowser();
	fProjectsFolderScroll = new BScrollView(B_TRANSLATE("Projects"),
		fProjectsFolderBrowser, B_FRAME_EVENTS | B_WILL_DRAW, true, true, B_FANCY_BORDER);
	fProjectsTabView->AddTab(fProjectsFolderScroll);

	// Source Control
	fSourceControlPanel = new SourceControlPanel();
	fProjectsTabView->AddTab(fSourceControlPanel);
}


void
GenioWindow::_InitWindow()
{
	_InitToolbar();

	_InitSideSplit();

	_InitCentralSplit();

	_InitOutputSplit();

	// Layout
	fRootLayout = BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(fMenuBar)
		.Add(fToolBar)

		.AddSplit(B_VERTICAL, 0.0f) // output split
		.SetInsets(-2.0f, 0.0f, -2.0f, -2.0f)
			.AddSplit(B_HORIZONTAL, 0.0f) // sidebar split
				.Add(fProjectsTabView, kProjectsWeight)
				.Add(fEditorTabsGroup, kEditorWeight)  // Editor
			.End() // sidebar split
			.Add(fOutputTabView, kOutputWeight)
		.End() //  output split
	;

	// Panels
	const char* projectsDirectory = gCFG["projects_directory"];
	BEntry entry(projectsDirectory, true);

	entry_ref ref;
	entry.GetRef(&ref);

	fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), &ref, B_FILE_NODE, true);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), &ref, B_FILE_NODE, false);

	BMessage *openProjectFolderMessage = new BMessage(MSG_PROJECT_FOLDER_OPEN);
	fOpenProjectFolderPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this),
												&ref, B_DIRECTORY_NODE, false,
												openProjectFolderMessage);

	fCreateNewProjectPanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this),
										&ref, B_DIRECTORY_NODE, false,
										new BMessage(MSG_CREATE_NEW_PROJECT),
										NULL, true, true);
}


void
GenioWindow::_MakeBindcatalogs()
{
	// Should not happen
	if (fActiveProject == nullptr)
		return;

	fBuildLogView->Clear();
	_ShowLog(kBuildLog);

	BMessage message;
	message.AddString("cmd", "make bindcatalogs");
	message.AddString("cmd_type", "bindcatalogs");

	// Go to appropriate directory
	chdir(fActiveProject->Path());

	fBuildLogView->RunCommand(&message);
}


void
GenioWindow::_MakeCatkeys()
{
	// Should not happen
	if (fActiveProject == nullptr)
		return;

	fBuildLogView->Clear();
	_ShowLog(kBuildLog);

	BMessage message;
	message.AddString("cmd", "make catkeys");
	message.AddString("cmd_type", "catkeys");

	// Go to appropriate directory
	chdir(fActiveProject->Path());

	fBuildLogView->RunCommand(&message);
}


/*
 * Creating a new project activates it, opening a project does not.
 * An active project closed does not activate one.
 * Reopen project at startup keeps active flag.
 * Activation could be set in projects outline context menu.
 */
void
GenioWindow::_ProjectFolderActivate(ProjectFolder *project)
{
	if (project == nullptr)
		return;

	// There is no active project
	if (fActiveProject == nullptr) {
		fActiveProject = project;
		project->SetActive(true);
		_UpdateProjectActivation(true);
	} else {
		// There was an active project already
		fActiveProject->SetActive(false);
		fActiveProject = project;
		project->SetActive(true);
		_UpdateProjectActivation(true);
	}

	_CollapseOrExpandProjects();

	if (!fDisableProjectNotifications) {
		BMessage noticeMessage(MSG_NOTIFY_PROJECT_SET_ACTIVE);
		noticeMessage.AddPointer("active_project", fActiveProject);
		noticeMessage.AddString("active_project_name", fActiveProject->Name());
		SendNotices(MSG_NOTIFY_PROJECT_SET_ACTIVE, &noticeMessage);
	}

	// Update run command working directory tooltip too
	BString tooltip;
	tooltip << "cwd: " << fActiveProject->Path();
	fRunConsoleProgramText->SetToolTip(tooltip);
}


void
GenioWindow::_ProjectFileDelete()
{
	BEntry entry(fProjectsFolderBrowser->GetSelectedProjectFileFullPath());
	entry_ref ref;
	entry.GetRef(&ref);
	if (!entry.Exists())
		return;

	char name[B_FILE_NAME_LENGTH];
	entry.GetName(name);

	BString text(B_TRANSLATE("Deleting item: \"%name%\".\n\n"
		"After deletion, the item will be lost.\n"
		"Do you really want to delete it?"));
	text.ReplaceFirst("%name%", name);

	BAlert* alert = new BAlert(B_TRANSLATE("Delete dialog"),
		text.String(),
		B_TRANSLATE("Cancel"), B_TRANSLATE("Delete"), nullptr,
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

	alert->SetShortcut(0, B_ESCAPE);

	int32 choice = alert->Go();

	if (choice == 0)
		return;
	else if (choice == 1) {
		// Close the file if open
		if (entry.IsFile()) {
			int32 openedIndex;
			if ((openedIndex = _GetEditorIndex(&ref)) != -1)
				_RemoveTab(openedIndex);
		}
		// Remove the entry
		if (entry.Exists()) {
			status_t status;
			if (entry.IsDirectory())
				status = FSDeleteFolder(&entry);
			else
				status = entry.Remove();
			if (status != B_OK) {
				BString text(B_TRANSLATE("Could not delete \"%name%\".\n\n"));
				text << ::strerror(status);
				text.ReplaceFirst("%name%", name);
				OKAlert("Delete item", text.String(), B_WARNING_ALERT);
				LogError("Could not delete %s (%s)", name, ::strerror(status));
			}
		}
	}
}


void
GenioWindow::_ProjectRenameFile()
{
	ProjectItem *item = fProjectsFolderBrowser->GetSelectedProjectItem();
	fProjectsFolderBrowser->InitRename(item);
}


// Project Folders
void
GenioWindow::_ProjectFolderClose(ProjectFolder *project)
{
	if (project == nullptr)
		return;

	std::vector<int32> unsavedFiles;
	for (int32 index = fTabManager->CountTabs() - 1 ; index > -1; index--) {
		Editor* editor = fTabManager->EditorAt(index);
		if (editor->IsModified())
			unsavedFiles.push_back(index);
	}

	if (!_FileRequestSaveList(unsavedFiles))
		return;

	BString closed("Project close:");
	BString name = project->Name();

	bool wasActive = false;
	// Active project closed
	if (project == fActiveProject) {
		wasActive = true;
		fActiveProject = nullptr;
		closed = "Active project close:";
		_UpdateProjectActivation(false);
		// Update run command working directory tooltip too
		BString tooltip;
		tooltip << "cwd: " << (const char*)gCFG["projects_directory"];
		fRunConsoleProgramText->SetToolTip(tooltip);
	}

	BString projectPath = project->Path();
	projectPath = projectPath.Append("/");

	for (int32 index = fTabManager->CountTabs() - 1 ; index > -1; index--) {
		Editor* editor = fTabManager->EditorAt(index);
		if (editor->GetProjectFolder() == project) {
			editor->SetProjectFolder(NULL);
			_RemoveTab(index);
		}
	}

	fProjectsFolderBrowser->ProjectFolderDepopulate(project);


	// Notify subscribers that the project list has changed
	if (!fDisableProjectNotifications)
		SendNotices(MSG_NOTIFY_PROJECT_LIST_CHANGED);

	project->Close();

	delete project;

	// Select a new active project
	if (wasActive) {
		ProjectItem* item = dynamic_cast<ProjectItem*>(fProjectsFolderBrowser->FullListItemAt(0));
		if (item != nullptr)
			_ProjectFolderActivate((ProjectFolder*)item->GetSourceItem());
	}

	// Disable "Close project" action if no project
	if (GetProjectBrowser()->CountProjects() == 0)
		ActionManager::SetEnabled(MSG_PROJECT_CLOSE, false);

	BString notification;
	notification << closed << " "  << name;
	LogInfo(notification.String());
}


status_t
GenioWindow::_ProjectFolderOpen(BMessage *message)
{
	entry_ref ref;
	status_t status = message->FindRef("refs", &ref);
	if (status == B_OK) {
		BPath path(&ref);
		status = _ProjectFolderOpen(path);
	} else {
		OKAlert("Open project folder", B_TRANSLATE("Invalid project folder"), B_STOP_ALERT);
	}

	return status;
}


status_t
GenioWindow::_ProjectFolderOpen(const BPath& path, bool activate)
{
	BEntry dirEntry(path.Path());
	if (!dirEntry.Exists())
		return B_NAME_NOT_FOUND;

	if (!dirEntry.IsDirectory())
		return B_NOT_A_DIRECTORY;

	// TODO: This avoids opening a volume as a project
	// Since the open operation is synchronous, this would take ages.
	// Not a complete fix, since if you open a folder with many other nested files/folders
	// it would still take ages.
	// Opening a volume seems wrong, anyway.
	if (BDirectory(&dirEntry).IsRootDirectory())
		return B_ERROR;

	entry_ref pathRef;
	status_t status = dirEntry.GetRef(&pathRef);
	if (status != B_OK)
		return status;

	// Check if already open
	for (int32 index = 0; index < GetProjectBrowser()->CountProjects(); index++) {
		ProjectFolder* pProject = static_cast<ProjectFolder*>(GetProjectBrowser()->ProjectAt(index));
		if (*pProject->EntryRef() == pathRef)
			return B_OK;
	}

	BMessenger msgr(this);
	ProjectFolder* newProject = new ProjectFolder(path.Path(), msgr);
	status = newProject->Open();
	if (status != B_OK) {
		BString notification;
		notification << "Project open fail: " << newProject->Name();
		LogInfo(notification.String());
		delete newProject;
		return status;
	}

	fProjectsFolderBrowser->ProjectFolderPopulate(newProject);


	// Notify subscribers that project list has changed
	if (!fDisableProjectNotifications)
		SendNotices(MSG_NOTIFY_PROJECT_LIST_CHANGED);

	_CollapseOrExpandProjects();

	BString opened("Project open: ");
	if (GetProjectBrowser()->CountProjects() == 1 || activate == true) {
		_ProjectFolderActivate(newProject);
		opened = "Active project open: ";
	}

	ActionManager::SetEnabled(MSG_PROJECT_CLOSE, true);

	BString notification;
	notification << opened << newProject->Name() << " at " << newProject->Path();
	LogInfo(notification.String());

	// let's check if any open editor is related to this project
	BString projectPath = newProject->Path();
	projectPath = projectPath.Append("/");

	for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
		Editor* editor = fTabManager->EditorAt(index);
		//LogError("Open project [%s] vs editor project [%s]", projectPath.String(), fEditor->FilePath().String());
		if (editor->GetProjectFolder() == NULL &&
		    editor->FilePath().StartsWith(projectPath)) {
			editor->SetProjectFolder(newProject);
		}
	}

    // final touch, let's be sure the folder is added to the recent files.
    be_roster->AddToRecentFolders(&pathRef, GenioNames::kApplicationSignature);

	return B_OK;
}


status_t
GenioWindow::_OpenTerminalWorkingDirectory()
{
	// TODO: return value is ignored: make it void ?
	ProjectItem* selectedProjectItem = fProjectsFolderBrowser->GetSelectedProjectItem();
	if (selectedProjectItem == nullptr)
		return B_BAD_VALUE;

	BString itemPath = selectedProjectItem->GetSourceItem()->Path();
	const char* argv[] = {
		"-w",
		itemPath.String(),
		nullptr
	};
	status_t status = be_roster->Launch("application/x-vnd.Haiku-Terminal", 2, argv);
	BString notification;
	if (status != B_OK) {
		notification <<
			"An error occurred while opening Terminal and setting working directory to: ";
		notification << itemPath << ": " << ::strerror(status);
		LogError(notification.String());
	} else {
		notification <<
			"Terminal successfully opened with working directory: ";
		notification << itemPath;
		LogTrace(notification.String());
	}

	return status;
}


status_t
GenioWindow::_ShowSelectedItemInTracker()
{
	// TODO: return value is ignored: make it void ?
	ProjectItem* selectedProjectItem = fProjectsFolderBrowser->GetSelectedProjectItem();
	if (selectedProjectItem == nullptr)
		return B_BAD_VALUE;

	BEntry itemEntry(selectedProjectItem->GetSourceItem()->Path());
	status_t status = itemEntry.InitCheck();
	if (status == B_OK) {
		BEntry parentDirectory;
		status = itemEntry.GetParent(&parentDirectory);
		if (status == B_OK) {
			entry_ref ref;
			status = parentDirectory.GetRef(&ref);
			if (status == B_OK)
				status = _ShowInTracker(ref);
		}
	}
	if (status != B_OK) {
		BString notification;
		notification << "An error occurred when showing an item in Tracker: " << ::strerror(status);
		LogError(notification.String());
	}
	return status;
}


status_t
GenioWindow::_ShowInTracker(const entry_ref& ref)
{
	BMessage message(B_EXECUTE_PROPERTY);
	status_t status = message.AddRef("data", &ref);
	if (status != B_OK)
		return status;
	status = message.AddSpecifier("Folder");
	if (status != B_OK)
		return status;

	BMessenger tracker("application/x-vnd.Be-TRAK");
	return tracker.SendMessage(&message);
}


int
GenioWindow::_Replace(int what)
{
	if (_ReplaceAllow() == false)
		return REPLACE_SKIP;

	BString selection(fFindTextControl->Text());
	BString replacement(fReplaceTextControl->Text());
	int retValue = REPLACE_NONE;

	Editor* editor = fTabManager->SelectedEditor();
	int flags = editor->SetSearchFlags(fFindCaseSensitiveCheck->Value(),
										fFindWholeWordCheck->Value(),
										false, false, false);

	bool wrap = fFindWrapCheck->Value();

	switch (what) {
		case REPLACE_ALL: {
			retValue = editor->ReplaceAll(selection, replacement, flags);
			editor->GrabFocus();
			break;
		}
		case REPLACE_NEXT: {
			retValue = editor->ReplaceAndFindNext(selection, replacement, flags, wrap);
			break;
		}
		case REPLACE_ONE: {
			retValue = editor->ReplaceOne(selection, replacement);
			break;
		}
		case REPLACE_PREVIOUS: {
			retValue = editor->ReplaceAndFindPrevious(selection, replacement, flags, wrap);
			break;
		}
		default:
			return REPLACE_NONE;
	}
	_UpdateFindMenuItems(fFindTextControl->Text());
	_UpdateReplaceMenuItems(fReplaceTextControl->Text());

	return retValue;
}


bool
GenioWindow::_ReplaceAllow() const
{
	BString selection(fFindTextControl->Text());
	BString replacement(fReplaceTextControl->Text());
	if (selection.Length() < 1
//			|| replacement.Length() < 1
			|| selection == replacement)
		return false;

	return true;
}


/*
void
GenioWindow::_ReplaceAndFind()
{
	if (_ReplaceAllow() == false)
		return;

	BString selection(fFindTextControl->Text());
	BString replacement(fReplaceTextControl->Text());
	editor = fEditorObjectList->ItemAt(fTabManager->SelectedTabIndex());

	int flags = editor->SetSearchFlags(fFindCaseSensitiveCheck->Value(),
										fFindWholeWordCheck->Value(),
										false, false, false);

	bool wrap = fFindWrapCheck->Value();

	editor->ReplaceAndFind(selection, replacement, flags, wrap);

	_UpdateFindMenuItems(fFindTextControl->Text());
	_UpdateReplaceMenuItems(fReplaceTextControl->Text());
}
*/


void
GenioWindow::_ReplaceGroupShow(bool show)
{
	bool findGroupOpen = !fFindGroup->IsHidden();

	if (findGroupOpen == false)
		_FindGroupShow(true);

	_ShowView(fReplaceGroup, show, MSG_REPLACE_GROUP_TOGGLED);

	if (!show) {
		fReplaceTextControl->TextView()->Clear();
		// If find group was not open focus and selection go there
		if (findGroupOpen == false)
			_GetFocusAndSelection(fFindTextControl);
		else
			_GetFocusAndSelection(fReplaceTextControl);
	}
	// Replace group was opened, get focus and selection
	else
		_GetFocusAndSelection(fReplaceTextControl);
}


status_t
GenioWindow::_RunInConsole(const BString& command)
{
	// If no active project go to projects directory
	if (fActiveProject == nullptr)
		chdir(gCFG["projects_directory"]);
	else
		chdir(fActiveProject->Path());

	_ShowLog(kOutputLog);

	BMessage message;
	message.AddString("cmd", command);
	message.AddString("cmd_type", command);

	return fConsoleIOView->RunCommand(&message);
}


void
GenioWindow::_RunTarget()
{
	// Should not happen
	if (fActiveProject == nullptr)
		return;

	chdir(fActiveProject->Path());

	// If there's no app just return, should not happen
	BEntry entry(fActiveProject->GetTarget());
	if (!entry.Exists())
		return;

	// Check if run args present
	BString args = fActiveProject->GetExecuteArgs();

	// Differentiate terminal projects from window ones
	if (fActiveProject->RunInTerminal() == true) {
		// Don't do that in graphical mode
		_UpdateProjectActivation(false);

		fConsoleIOView->Clear();
		_ShowLog(kOutputLog);

		BString command;
		if ((bool)gCFG["run_without_buffering"]) {
			command << "stdbuf -i0 -e0 -o0 ";
		}
		command << fActiveProject->GetTarget();
		if (!args.IsEmpty())
			command << " " << args;
		// TODO: Go to appropriate directory
		// chdir(...);

		BString claim("Run ");
		claim << fActiveProject->Name();
		claim << " (";
		claim << (fActiveProject->GetBuildMode() == BuildMode::ReleaseMode ? B_TRANSLATE("Release") : B_TRANSLATE("Debug"));
		claim << ")";

		GMessage message = {{"cmd", command},
							{"cmd_type", "build"},
							{"banner_claim", claim }};

		fConsoleIOView->MakeFocus(true);

		fConsoleIOView->RunCommand(&message);

	} else {
		argv_split parser(fActiveProject->GetTarget().String());
		parser.parse(fActiveProject->GetExecuteArgs().String());

		entry_ref ref;
		entry.SetTo(fActiveProject->GetTarget());
		entry.GetRef(&ref);
		be_roster->Launch(&ref, parser.getArguments().size() , parser.argv());
	}
}


void
GenioWindow::_SetMakefileBuildMode()
{
}


void
GenioWindow::_ShowLog(int32 index)
{
	if (fOutputTabView->IsHidden())
		fOutputTabView ->Show();

	fOutputTabView->Select(index);
}


void
GenioWindow::_UpdateFindMenuItems(const BString& text)
{
	int32 count = fFindMenuField->Menu()->CountItems();
	// Add item if not already present
	if (fFindMenuField->Menu()->FindItem(text) == nullptr) {
		BMenuItem* item = new BMenuItem(text, new BMessage(MSG_FIND_MENU_SELECTED));
		fFindMenuField->Menu()->AddItem(item, 0);
	}
	if (count == kFindReplaceMenuItems)
		fFindMenuField->Menu()->RemoveItem(count);
}


status_t
GenioWindow::_UpdateLabel(int32 index, bool isModified)
{
	if (index > -1 && index < fTabManager->CountTabs()) {
		if (isModified == true) {
				// Add '*' to file name
				BString label(fTabManager->TabLabel(index));
				label.Append("*");
				fTabManager->SetTabLabel(index, label.String());
		} else {
				// Remove '*' from file name
				BString label(fTabManager->TabLabel(index));
				label.RemoveLast("*");
				fTabManager->SetTabLabel(index, label.String());
		}
		return B_OK;
	}

	return B_ERROR;
}


void
GenioWindow::_UpdateProjectActivation(bool active)
{
	ActionManager::SetEnabled(MSG_CLEAN_PROJECT, active);
	fBuildModeItem->SetEnabled(active);
	fMakeCatkeysItem->SetEnabled(active);
	fMakeBindcatalogsItem->SetEnabled(active);
	ActionManager::SetEnabled(MSG_BUILD_PROJECT, active);
	fFileNewMenuItem->SetEnabled(true); // This menu should be always active!

	if (active == true) {
		// Is this a git project?
		try {
			if (fActiveProject->GetRepository()->IsInitialized())
				fGitMenu->SetEnabled(true);
			else
				fGitMenu->SetEnabled(false);
		} catch (const GitException &ex) {
		}

		// Build mode
		bool releaseMode = (fActiveProject->GetBuildMode() == BuildMode::ReleaseMode);

		fDebugModeItem->SetMarked(!releaseMode);
		fReleaseModeItem->SetMarked(releaseMode);

		ActionManager::SetEnabled(MSG_PROJECT_SETTINGS, true);

		// Target exists: enable run button
		chdir(fActiveProject->Path());
		BEntry entry(fActiveProject->GetTarget());
		if (entry.Exists()) {
			ActionManager::SetEnabled(MSG_RUN_TARGET, true);
			ActionManager::SetEnabled(MSG_DEBUG_PROJECT, !releaseMode);
		} else {
			ActionManager::SetEnabled(MSG_RUN_TARGET, false);
			ActionManager::SetEnabled(MSG_DEBUG_PROJECT, false);
		}
	} else { // here project is inactive
		fGitMenu->SetEnabled(false);
		ActionManager::SetEnabled(MSG_RUN_TARGET, false);
		ActionManager::SetEnabled(MSG_DEBUG_PROJECT, false);
		ActionManager::SetEnabled(MSG_PROJECT_SETTINGS, false);
		fFileNewMenuItem->SetViewMode(TemplatesMenu::ViewMode::SHOW_ALL_VIEW_MODE);
	}

	fProjectsFolderBrowser->Invalidate();
}


void
GenioWindow::_UpdateReplaceMenuItems(const BString& text)
{
	int32 items = fReplaceMenuField->Menu()->CountItems();
	// Add item if not already present
	if (fReplaceMenuField->Menu()->FindItem(text) == NULL) {
		BMenuItem* item = new BMenuItem(text, new BMessage(MSG_REPLACE_MENU_SELECTED));
		fReplaceMenuField->Menu()->AddItem(item, 0);
	}
	if (items == kFindReplaceMenuItems)
		fReplaceMenuField->Menu()->RemoveItem(items);
}


// Called by:
// Savepoint Reached
// Savepoint Left
// Undo
// Redo
// EDITOR_POSITION_CHANGED (Why ?)
void
GenioWindow::_UpdateSavepointChange(Editor* editor, const BString& caller)
{
	assert (editor);

	// Menu Items
	ActionManager::SetEnabled(B_CUT, editor->CanCut());
	ActionManager::SetEnabled(B_COPY, editor->CanCopy());
	ActionManager::SetEnabled(B_PASTE, editor->CanPaste());

	ActionManager::SetEnabled(B_UNDO, editor->CanUndo());
	ActionManager::SetEnabled(B_REDO, editor->CanRedo());

	//ActionManager.
	ActionManager::SetEnabled(MSG_FILE_SAVE, editor->IsModified());

	// editor is modified by _FilesNeedSave so it should be the last
	// or reload editor pointer
	bool filesNeedSave = (_FilesNeedSave() > 0 ? true : false);
	ActionManager::SetEnabled(MSG_FILE_SAVE_ALL, filesNeedSave);

	// Avoid notifiying listeners for every position change
	// at least the ProjectFolderBrowser would invalidate() itself
	// every time. Not needed for this case.
	// TODO: is that even correct to be called for every position changed ?
	if (caller != "EDITOR_POSITION_CHANGED") {
		BMessage noticeMessage(MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED);
		noticeMessage.AddString("file_name", editor->FilePath());
		noticeMessage.AddBool("needs_save", editor->IsModified());
		SendNotices(MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED, &noticeMessage);
	}
}


// Updating menu, toolbar, title.
// Also cleaning Status bar if no open files
void
GenioWindow::_UpdateTabChange(Editor* editor, const BString& caller)
{
	// All files are closed
	if (editor == nullptr) {
		// ToolBar Items
		//_FindGroupShow(false);
		ActionManager::SetEnabled(MSG_FILE_FOLD_TOGGLE, false);
		ActionManager::SetEnabled(B_UNDO, false);
		ActionManager::SetEnabled(B_REDO, false);

		ActionManager::SetEnabled(MSG_FILE_SAVE, false);
		ActionManager::SetEnabled(MSG_FILE_SAVE_AS, false);
		ActionManager::SetEnabled(MSG_FILE_SAVE_ALL, false);
		ActionManager::SetEnabled(MSG_FILE_CLOSE, false);
		ActionManager::SetEnabled(MSG_FILE_CLOSE_ALL, false);

		ActionManager::SetEnabled(MSG_BUFFER_LOCK, false);
		fToolBar->SetActionEnabled(MSG_FILE_MENU_SHOW, false);

		ActionManager::SetEnabled(MSG_FILE_NEXT_SELECTED, false);
		ActionManager::SetEnabled(MSG_FILE_PREVIOUS_SELECTED, false);

		ActionManager::SetEnabled(B_CUT, false);
		ActionManager::SetEnabled(B_COPY, false);
		ActionManager::SetEnabled(B_PASTE, false);
		ActionManager::SetEnabled(B_SELECT_ALL, false);

		ActionManager::SetEnabled(MSG_TEXT_OVERWRITE, false);
		ActionManager::SetEnabled(MSG_WHITE_SPACES_TOGGLE, false);
		ActionManager::SetEnabled(MSG_LINE_ENDINGS_TOGGLE, false);

		ActionManager::SetEnabled(MSG_DUPLICATE_LINE, false);
		ActionManager::SetEnabled(MSG_DELETE_LINES, false);
		ActionManager::SetEnabled(MSG_COMMENT_SELECTED_LINES, false);
		ActionManager::SetEnabled(MSG_FILE_TRIM_TRAILING_SPACE, false);

		ActionManager::SetEnabled(MSG_AUTOCOMPLETION, false);
		ActionManager::SetEnabled(MSG_FORMAT, false);
		ActionManager::SetEnabled(MSG_GOTODEFINITION, false);
		ActionManager::SetEnabled(MSG_GOTODECLARATION, false);
		ActionManager::SetEnabled(MSG_GOTOIMPLEMENTATION, false);
		ActionManager::SetEnabled(MSG_SWITCHSOURCE, false);

		fLineEndingsMenu->SetEnabled(false);
		fLanguageMenu->SetEnabled(false);
		ActionManager::SetEnabled(MSG_FIND_NEXT, false);
		ActionManager::SetEnabled(MSG_FIND_PREVIOUS, false);
		ActionManager::SetEnabled(MSG_FIND_MARK_ALL, false);
		fReplaceGroup->SetEnabled(false);

		ActionManager::SetEnabled(MSG_GOTO_LINE, false);
		fBookmarksMenu->SetEnabled(false);

		_UpdateWindowTitle(nullptr);

		fProblemsPanel->ClearProblems();
		return;
	}

	// ToolBar Items

	ActionManager::SetEnabled(MSG_FILE_FOLD_TOGGLE, editor->IsFoldingAvailable());
	ActionManager::SetEnabled(B_UNDO, editor->CanUndo());
	ActionManager::SetEnabled(B_REDO, editor->CanRedo());
	ActionManager::SetEnabled(MSG_FILE_SAVE, editor->IsModified());
	ActionManager::SetEnabled(MSG_FILE_CLOSE, true);

	ActionManager::SetEnabled(MSG_BUFFER_LOCK, true);
	ActionManager::SetPressed(MSG_BUFFER_LOCK, editor->IsReadOnly());
	fToolBar->SetActionEnabled(MSG_FILE_MENU_SHOW, true);
	// Arrows
	int32 maxTabIndex = (fTabManager->CountTabs() - 1);
	int32 index = fTabManager->SelectedTabIndex();

	ActionManager::SetEnabled(MSG_FILE_PREVIOUS_SELECTED, index > 0);
	ActionManager::SetEnabled(MSG_FILE_NEXT_SELECTED, maxTabIndex > index);

	// Menu Items
	ActionManager::SetEnabled(MSG_FILE_SAVE_AS, true);
	ActionManager::SetEnabled(MSG_FILE_CLOSE_ALL, true);


	ActionManager::SetEnabled(B_CUT, editor->CanCut());
	ActionManager::SetEnabled(B_COPY, editor->CanCopy());
	ActionManager::SetEnabled(B_PASTE, editor->CanPaste());
	ActionManager::SetEnabled(B_SELECT_ALL, true);

	ActionManager::SetEnabled(MSG_TEXT_OVERWRITE, true);

	ActionManager::SetEnabled(MSG_WHITE_SPACES_TOGGLE, true);
	ActionManager::SetEnabled(MSG_LINE_ENDINGS_TOGGLE, true);

	fLineEndingsMenu->SetEnabled(!editor->IsReadOnly());
	fLanguageMenu->SetEnabled(true);
	//Setting the right message type:
	std::string languageName = Languages::GetMenuItemName(editor->FileType());
	for (int32 i=0;i<fLanguageMenu->CountItems();i++) {
		if (languageName.compare(fLanguageMenu->ItemAt(i)->Label()) == 0)  {
			//fLanguageMenu->
			fLanguageMenu->ItemAt(i)->SetMarked(true);
		}
	}

	ActionManager::SetEnabled(MSG_DUPLICATE_LINE, !editor->IsReadOnly());
	ActionManager::SetEnabled(MSG_DELETE_LINES, !editor->IsReadOnly());
	ActionManager::SetEnabled(MSG_COMMENT_SELECTED_LINES, !editor->IsReadOnly());
	ActionManager::SetEnabled(MSG_FILE_TRIM_TRAILING_SPACE, !editor->IsReadOnly());

	ActionManager::SetEnabled(MSG_AUTOCOMPLETION, !editor->IsReadOnly() && editor->HasLSPServer());
	ActionManager::SetEnabled(MSG_FORMAT, !editor->IsReadOnly() && editor->HasLSPServer());
	ActionManager::SetEnabled(MSG_GOTODEFINITION, editor->HasLSPServer());
	ActionManager::SetEnabled(MSG_GOTODECLARATION, editor->HasLSPServer());
	ActionManager::SetEnabled(MSG_GOTOIMPLEMENTATION, editor->HasLSPServer());
	ActionManager::SetEnabled(MSG_SWITCHSOURCE, (editor->FileType().compare("cpp") == 0));

	ActionManager::SetEnabled(MSG_FIND_NEXT, true);
	ActionManager::SetEnabled(MSG_FIND_PREVIOUS, true);
	ActionManager::SetEnabled(MSG_FIND_MARK_ALL, true);
	fReplaceGroup->SetEnabled(true);
	ActionManager::SetEnabled(MSG_GOTO_LINE, true);

	fBookmarksMenu->SetEnabled(true);

	_UpdateWindowTitle(editor->FilePath().String());

	// editor is modified by _FilesNeedSave so it should be the last
	// or reload editor pointer
	bool filesNeedSave = _FilesNeedSave();
	ActionManager::SetEnabled(MSG_FILE_SAVE_ALL, filesNeedSave);

	BMessage diagnostics;
	editor->GetProblems(&diagnostics);
	fProblemsPanel->UpdateProblems(&diagnostics);

	LogTraceF("called by: %s:%d", caller.String(), index);
}


void
GenioWindow::_HandleConfigurationChanged(BMessage* message)
{
	// TODO: Should editors handle these things on their own ?
	BString key ( message->GetString("key", "") );
	if (key.IsEmpty())
		return;

	// TODO: apply other settings
	for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
		Editor* editor = fTabManager->EditorAt(index);
		editor->ApplySettings();
	}

	if (key.StartsWith("find_")) {
		fFindWrapCheck->SetValue(gCFG["find_wrap"] ? B_CONTROL_ON : B_CONTROL_OFF);
		fFindWholeWordCheck->SetValue(gCFG["find_whole_word"] ? B_CONTROL_ON : B_CONTROL_OFF);
		fFindCaseSensitiveCheck->SetValue(gCFG["find_match_case"] ? B_CONTROL_ON : B_CONTROL_OFF);
	}

	_CollapseOrExpandProjects();

	Editor* selected = fTabManager->SelectedEditor();
	_UpdateWindowTitle(selected ? selected->FilePath().String() : nullptr);
}


void
GenioWindow::UpdateMenu()
{
	ProjectItem *item = fProjectsFolderBrowser->GetSelectedProjectItem();
	if (item != nullptr) {
		if (item->GetSourceItem()->Type() != SourceItemType::FileItem)
			fFileNewMenuItem->SetViewMode(TemplatesMenu::ViewMode::SHOW_ALL_VIEW_MODE);
		else
			fFileNewMenuItem->SetViewMode(TemplatesMenu::ViewMode::DISABLE_FILES_VIEW_MODE, false);
	}
}


void
GenioWindow::_UpdateWindowTitle(const char* filePath)
{
	BString title;
	title << GenioNames::kApplicationName;
	// File full path in window title
	if (gCFG["fullpath_title"] && filePath != nullptr)
		title << ": " << filePath;
	SetTitle(title.String());
}


void
GenioWindow::_CollapseOrExpandProjects()
{
	if (gCFG["auto_expand_collapse_projects"]) {
		// Expand active project, collapse other
		// TODO: Improve by adding necessary APIs to ProjectFolderBrowser
		for (int32 i = 0; i < fProjectsFolderBrowser->CountItems(); i++) {
			if ((ProjectFolder*)fProjectsFolderBrowser->GetProjectItemAt(i)->GetSourceItem() == fActiveProject)
				fProjectsFolderBrowser->Expand(fProjectsFolderBrowser->GetProjectItemAt(i));
			else
				fProjectsFolderBrowser->Collapse(fProjectsFolderBrowser->GetProjectItemAt(i));
		}
	}
}
