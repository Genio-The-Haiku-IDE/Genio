/*
 * Copyright 2022-2023 Nexus6 <nexus6.haiku@icloud.com>
 * Copyright 2017..2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GenioWindow.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <Alert.h>
#include <Application.h>
#include <Architecture.h>
#include <Catalog.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <RecentItems.h>
#include <Resources.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <StringFormat.h>
#include <StringItem.h>
#include <NodeInfo.h>

#include <DirMenu.h>


#include "exceptions/Exceptions.h"
#include "FSUtils.h"
#include "GenioCommon.h"
#include "GenioNamespace.h"
#include "GenioWindowMessages.h"
#include "Log.h"
#include "ProjectSettingsWindow.h"
#include "ProjectFolder.h"
#include "ProjectItem.h"
#include "SettingsWindow.h"
#include "TemplatesMenu.h"
#include "TemplateManager.h"
#include "TPreferences.h"
#include "TextUtils.h"
#include "Utils.h"
#include "EditorKeyDownMessageFilter.h"

#include "ActionManager.h"
#include "QuitAlert.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioWindow"


constexpr auto kRecentFilesNumber = 14 + 1;

//static constexpr float kTabBarHeight = 35.0f;


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
	, fProjectFolderObjectList(nullptr)
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
{
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
	if (GenioNames::Settings.reopen_projects == true) {
		TPreferences projects(GenioNames::kSettingsProjectsToReopen,
								GenioNames::kApplicationName, 'PRRE');
		if (!projects.IsEmpty()) {
			BString projectName, activeProject = "";

			activeProject = projects.GetString("active_project");
			for (auto count = 0; projects.FindString("project_to_reopen",
										count, &projectName) == B_OK; count++)
					_ProjectFolderOpen(projectName, projectName == activeProject);
		}
	}

	// Reopen files
	if (GenioNames::Settings.reopen_files == true) {
		TPreferences files(GenioNames::kSettingsFilesToReopen,
							GenioNames::kApplicationName, 'FIRE');
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

		_ShowView(fProjectsTabView, GenioNames::Settings.show_projects, MSG_SHOW_HIDE_PROJECTS);
		_ShowView(fOutputTabView,   GenioNames::Settings.show_output,	MSG_SHOW_HIDE_OUTPUT);
		_ShowView(fToolBar,  		GenioNames::Settings.show_toolbar,	MSG_TOGGLE_TOOLBAR);

		UnlockLooper();
	}
}

GenioWindow::~GenioWindow()
{
	delete fTabManager;
	delete fOpenPanel;
	delete fSavePanel;
	delete fOpenProjectFolderPanel;
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
		case MSG_ESCAPE_KEY:{
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
		}
		break;
		case EDITOR_UPDATE_DIAGNOSTICS : {
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				int32 index = _GetEditorIndex(&ref);
				if (index == fTabManager->SelectedTabIndex())
				{
					fProblemsPanel->UpdateProblems(message);
				}
			}
			break;
		}
		case 'NOTI': {
			BString notification, type;
			if (message->FindString("notification", &notification) == B_OK
				&& message->FindString("type", &type) == B_OK) {
					_SendNotification(notification, type);
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
		case B_REDO: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				if (editor->CanRedo())
					editor->Redo();
				_UpdateSavepointChange(fTabManager->SelectedTabIndex(), "Redo");
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
				_UpdateSavepointChange(fTabManager->SelectedTabIndex(), "Undo");
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
				fProjectsFolderBrowser->SetBuildingPhase(fIsBuilding);

			}
			_UpdateProjectActivation(fActiveProject != nullptr);
			break;
		}
		case EDITOR_FIND_SET_MARK: {
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				int32 index = _GetEditorIndex(&ref);
				if (index == fTabManager->SelectedTabIndex()) {
					Editor* editor = fTabManager->EditorAt(index);

					int32 line;
					if (message->FindInt32("line", &line) == B_OK) {
						BString text;
						text << editor->Name() << " :" << line;
						_SendNotification(text, "FIND_MARK");
					}
				}
			}
			break;
		}
		case EDITOR_FIND_NEXT_MISS: {
			_SendNotification("Find next not found", "FIND_MISS");
			break;
		}
		case EDITOR_FIND_PREV_MISS: {
			_SendNotification("Find previous not found", "FIND_MISS");
			break;
		}
		case EDITOR_FIND_COUNT: {
			int32 count;
			BString text;
			if (message->FindString("text_to_find", &text) == B_OK
				&& message->FindInt32("count", &count) == B_OK) {

				BString notification;
				notification << "\"" << text << "\""
					<< " occurrences found: " << count;

				_ShowLog(kNotificationLog);
				_SendNotification(notification, "FIND_COUNT");
			}
			break;
		}

		case EDITOR_REPLACE_ALL_COUNT: {
			int32 count;
			if (message->FindInt32("count", &count) == B_OK) {
				BString notification;
				notification << "Replacements done: " << count;

				_ShowLog(kNotificationLog);
				_SendNotification(notification, "REPL_COUNT");
			}
			break;
		}
		case EDITOR_REPLACE_ONE: {
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				int32 index =  _GetEditorIndex(&ref);
				if (index == fTabManager->SelectedTabIndex()) {
					Editor* editor = fTabManager->EditorAt(index);
					int32 line, column;
					BString sel, repl;
					if (message->FindInt32("line", &line) == B_OK
						&& message->FindInt32("column", &column) == B_OK
						&& message->FindString("selection", &sel) == B_OK
						&& message->FindString("replacement", &repl) == B_OK) {
						BString notification;
						notification << editor->Name() << " " << line  << ":" << column
							 << " \"" << sel << "\" => \""<< repl<< "\"";

						_ShowLog(kNotificationLog);
						_SendNotification(notification, "REPL_DONE");
					}
				}
			}
			break;
		}

		case EDITOR_POSITION_CHANGED: {
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				int32 index =  _GetEditorIndex(&ref);
				if (index == fTabManager->SelectedTabIndex()) {
					// Enable Cut,Copy,Paste shortcuts
					_UpdateSavepointChange(index, "EDITOR_POSITION_CHANGED");
				}
			}
			break;
		}
		case EDITOR_UPDATE_SAVEPOINT: {
			entry_ref ref;
			bool modified = false;
			if (message->FindRef("ref", &ref) == B_OK &&
			    message->FindBool("modified", &modified) == B_OK) {

				int32 index = _GetEditorIndex(&ref);
				if (index > -1) {
					_UpdateLabel(index, modified);
					_UpdateSavepointChange(index, "UpdateSavePoint");
				}
			}

			break;
		}
		case MSG_BOOKMARK_CLEAR_ALL: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->BookmarkClearAll(sci_BOOKMARK);
			}
			break;
		}
		case MSG_BOOKMARK_GOTO_NEXT: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				if (!editor->BookmarkGoToNext())
					_SendNotification("Next Bookmark not found", "FIND_MISS");
			}

			break;
		}
		case MSG_BOOKMARK_GOTO_PREVIOUS: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				if (!editor->BookmarkGoToPrevious())
					_SendNotification("Previous Bookmark not found", "FIND_MISS");
			}


			break;
		}
		case MSG_BOOKMARK_TOGGLE: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->BookmarkToggle(editor->GetCurrentPosition());
			}
			break;
		}
		case MSG_BUFFER_LOCK: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->SetReadOnly(!editor->IsReadOnly());
				_UpdateTabChange(editor, "Buffer Lock");
			}
			break;
		}
		case MSG_BUILD_MODE_DEBUG: {

			fActiveProject->SetBuildMode(BuildMode::DebugMode);
			fActiveProject->SaveSettings();
			_UpdateProjectActivation(fActiveProject != nullptr);
			break;
		}
		case MSG_BUILD_MODE_RELEASE: {

			fActiveProject->SetBuildMode(BuildMode::ReleaseMode);
			fActiveProject->SaveSettings();
			_UpdateProjectActivation(fActiveProject != nullptr);
			break;
		}
		case MSG_BUILD_PROJECT: {
			_BuildProject();
			break;
		}
		case MSG_CLEAN_PROJECT: {
			_CleanProject();
			break;
		}
		case MSG_DEBUG_PROJECT: {
			_DebugProject();
			break;
		}
		case MSG_EOL_CONVERT_TO_UNIX: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->EndOfLineConvert(SC_EOL_LF);
			}
			break;
		}
		case MSG_EOL_CONVERT_TO_DOS: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->EndOfLineConvert(SC_EOL_CRLF);
			}
			break;
		}
		case MSG_EOL_CONVERT_TO_MAC: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->EndOfLineConvert(SC_EOL_CR);
			}
			break;
		}
		case MSG_FILE_CLOSE: {
			_FileRequestClose(fTabManager->SelectedTabIndex());
			break;
		}
		case MSG_FILE_CLOSE_ALL:
			_FileCloseAll();
			break;
		case MSG_FILE_FOLD_TOGGLE: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->ToggleFolding();
			}
			break;
		}
		case MSG_FILE_MENU_SHOW: {
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
		case MSG_FILE_NEXT_SELECTED: {
			int32 index = fTabManager->SelectedTabIndex();

			if (index < fTabManager->CountTabs() - 1)
				fTabManager->SelectTab(index + 1);
			break;
		}
		case MSG_FILE_OPEN:
			fOpenPanel->Show();
			break;
		case MSG_FILE_PREVIOUS_SELECTED: {
			int32 index = fTabManager->SelectedTabIndex();

			if (index > 0 && index < fTabManager->CountTabs())
				fTabManager->SelectTab(index - 1);
			break;
		}
		case MSG_FILE_SAVE:
			_FileSave(fTabManager->SelectedTabIndex());
			break;
		case MSG_FILE_SAVE_AS: {
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
			if (GenioNames::Settings.editor_zoom < 20) {
				GenioNames::Settings.editor_zoom++;
				for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
					Editor* editor = fTabManager->EditorAt(index);
					editor->SetZoom(GenioNames::Settings.editor_zoom);
				}
			}
		break;
		case MSG_VIEW_ZOOMOUT:
			if (GenioNames::Settings.editor_zoom > -10) {
				GenioNames::Settings.editor_zoom--;
				for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
					Editor* editor = fTabManager->EditorAt(index);
					editor->SetZoom(GenioNames::Settings.editor_zoom);
				}
			}
		break;
		case MSG_VIEW_ZOOMRESET:
			GenioNames::Settings.editor_zoom = 0;
			for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
				Editor* editor = fTabManager->EditorAt(index);
				editor->SetZoom(GenioNames::Settings.editor_zoom);
			}
		break;
		case MSG_FIND_GROUP_SHOW:
			_FindGroupShow(true);
			break;
		case MSG_FIND_IN_FILES: {
			_FindInFiles();
			break;
		}
		case MSG_FIND_MARK_ALL: {
			BString textToFind(fFindTextControl->Text());

			if (!textToFind.IsEmpty()) {
				_FindMarkAll(textToFind);
			}
			break;
		}
		case MSG_FIND_MENU_SELECTED: {
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				BMenuItem* item = fFindMenuField->Menu()->ItemAt(index);
				fFindTextControl->SetText(item->Label());
			}
			break;
		}
		case MSG_FIND_INVOKED: {
			if (CurrentFocus() == fFindTextControl->TextView()) {
				const BString& text(fFindTextControl->Text());
				_FindNext(text, false);
				fFindTextControl->MakeFocus(true);
			}
			break;
		}
		case MSG_FIND_NEXT: {
			const BString& text(fFindTextControl->Text());
			_FindNext(text, false);
			break;
		}
		case MSG_FIND_PREVIOUS: {
			const BString& text(fFindTextControl->Text());
			_FindNext(text, true);
			break;
		}
		case MSG_FIND_GROUP_TOGGLED:
			_FindGroupShow(fFindGroup->IsHidden());
			break;
		case MSG_GIT_COMMAND: {
			BString command;
			if (message->FindString("command", &command) == B_OK)
				_Git(command);
			break;
		}
		case GTLW_GO: {
			int32 line;
			if(message->FindInt32("line", &line) == B_OK) {
				Editor* editor = fTabManager->SelectedEditor();
				editor->GoToLine(line);
			}
		} break;
		case MSG_GOTO_LINE:
			if(fGoToLineWindow == nullptr) {
				fGoToLineWindow = new GoToLineWindow(this);
			}
			fGoToLineWindow->ShowCentered(Frame());
			break;
		case MSG_LINE_ENDINGS_TOGGLE: {
			GenioNames::Settings.show_line_endings = !GenioNames::Settings.show_line_endings;
			for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
				Editor* editor = fTabManager->EditorAt(index);
				editor->ShowLineEndings(GenioNames::Settings.show_line_endings);
			}
			ActionManager::SetPressed(MSG_LINE_ENDINGS_TOGGLE, GenioNames::Settings.show_line_endings);
			break;
		}
		case MSG_DUPLICATE_LINE: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->DuplicateCurrentLine();
			}
			break;
		}
		case MSG_DELETE_LINES: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->DeleteSelectedLines();
			}
			break;
		}
		case MSG_COMMENT_SELECTED_LINES: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->CommentSelectedLines();
			}
			break;
		}
		case MSG_FILE_TRIM_TRAILING_SPACE: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->TrimTrailingWhitespace();
			}
			break;
		}
		case MSG_AUTOCOMPLETION: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor)
				editor->Completion();
			break;
		}
		case MSG_FORMAT: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor)
				editor->Format();
			break;
		}

		case MSG_GOTODEFINITION: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor)
				editor->GoToDefinition();

			break;
		}
		case MSG_GOTODECLARATION: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor)
				editor->GoToDeclaration();

			break;
		}
		case MSG_GOTOIMPLEMENTATION: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor)
				editor->GoToImplementation();
			break;
		}
		case MSG_SWITCHSOURCE: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor)
				editor->SwitchSourceHeader();

			break;
		}

		case MSG_MAKE_BINDCATALOGS: {
			_MakeBindcatalogs();
			break;
		}
		case MSG_MAKE_CATKEYS: {
			_MakeCatkeys();
			break;
		}
		case MSG_PROJECT_CLOSE: {
			_ProjectFolderClose(fActiveProject);
			break;
		}
		case MSG_SHOW_TEMPLATE_USER_FOLDER:
		{
			entry_ref new_ref;
			status_t status = message->FindRef("refs", &new_ref);
			if (status != B_OK) {
				LogError("Can't find ref in message!");
			} else {
				_ShowInTracker(&new_ref);
			}
		}
		break;
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
				_ProjectFolderOpen(path.Path());
			} else {
				LogError("TemplateManager: could create %s from %s to %s",
							name.String(), template_ref.name, dest_ref.name);
			}
		}
		break;
		case MSG_FILE_NEW:
		case MSG_PROJECT_MENU_NEW_FILE:
		{
			status_t status;

			BString type;
			status = message->FindString("type", &type);
			if (status != B_OK) {
				LogError("Can't find type!");
				return;
			}

			if (type == "new_folder") {
				ProjectItem* item = fProjectsFolderBrowser->GetCurrentProjectItem();
				if (item && item->GetSourceItem()->Type() != SourceItemType::FileItem) {
					BEntry entry(item->GetSourceItem()->Path());
					entry_ref ref;
					if (entry.GetRef(&ref) != B_OK) {
						LogError("Invalid path [%s]", item->GetSourceItem()->Path().String());
						return;
					} else {
						status = TemplateManager::CreateNewFolder(&ref);
						if (status != B_OK) {
							OKAlert(B_TRANSLATE("New folder"),
									B_TRANSLATE("Error creating folder"),
									B_WARNING_ALERT);
							LogError("Invalid destination directory [%s]", entry.Name());
						}
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
				ProjectItem* item = fProjectsFolderBrowser->GetCurrentProjectItem();
				if (item && item->GetSourceItem()->Type() != SourceItemType::FileItem) {
					BEntry entry(item->GetSourceItem()->Path());
					if (entry.GetRef(&dest) != B_OK) {
						LogError("Invalid path [%s]", item->GetSourceItem()->Path().String());
						return;
					}
					if (message->FindRef("refs", &source) != B_OK) {
						LogError("Can't find ref in message!");
						return;
					}
					status_t status = TemplateManager::CopyFileTemplate(&source, &dest);
					if (status != B_OK) {
						OKAlert(B_TRANSLATE("New file"),
								B_TRANSLATE("Could not create a new file"),
								B_WARNING_ALERT);
						LogError("Invalid destination directory [%s]", entry.Name());
						return;
					}
				}
			}
		}
		break;
		case MSG_PROJECT_MENU_CLOSE: {
			_ProjectFolderClose(fProjectsFolderBrowser->GetProjectFromCurrentItem());
			break;
		}
		case MSG_PROJECT_MENU_DELETE_FILE: {
			_ProjectFileDelete();
			break;
		}
		case MSG_PROJECT_MENU_RENAME_FILE: {
			_ProjectRenameFile();
			break;
		}
		case MSG_PROJECT_MENU_SHOW_IN_TRACKER: {
			_ShowCurrentItemInTracker();
			break;
		}
		case MSG_PROJECT_MENU_OPEN_TERMINAL: {
			_OpenTerminalWorkingDirectory();
			break;
		}
		case MSG_PROJECT_MENU_SET_ACTIVE: {
			_ProjectFolderActivate(fProjectsFolderBrowser->GetProjectFromCurrentItem());
			break;
		}
		case MSG_PROJECT_OPEN: {
			fOpenProjectFolderPanel->Show();
			break;
		}
		case MSG_PROJECT_SETTINGS: {
			if (fActiveProject != nullptr) {
				ProjectSettingsWindow *window = new ProjectSettingsWindow(fActiveProject);
				window->Show();
			}
			break;
		}
		case MSG_PROJECT_FOLDER_OPEN: {
			_ProjectFolderOpen(message);
			break;
		}
		case MSG_REPLACE_GROUP_SHOW:
			_ReplaceGroupShow(true);
			break;
		case MSG_REPLACE_ALL: {
			_Replace(REPLACE_ALL);
			break;
		}
		case MSG_REPLACE_GROUP_TOGGLED:
			_ReplaceGroupShow(fReplaceGroup->IsHidden());
			break;
		case MSG_REPLACE_MENU_SELECTED: {
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
		case MSG_RUN_CONSOLE_PROGRAM_SHOW: {
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
		case MSG_RUN_CONSOLE_PROGRAM: {
			const BString& command(fRunConsoleProgramText->Text());
			if (!command.IsEmpty())
				_RunInConsole(command);
			fRunConsoleProgramText->SetText("");
			break;
		}
		case MSG_RUN_TARGET:
			_RunTarget();
			break;
		case MSG_SELECT_TAB: {
			int32 index;
			// Shortcut selection, be careful
			if (message->FindInt32("index", &index) == B_OK) {
				if (index < fTabManager->CountTabs()
					&& index != fTabManager->SelectedTabIndex())
					fTabManager->SelectTab(index);
			}
			break;
		}
		case MSG_SHOW_HIDE_PROJECTS: {
			_ShowView(fProjectsTabView, fProjectsTabView->IsHidden(), MSG_SHOW_HIDE_PROJECTS);
			break;
		}
		case MSG_SHOW_HIDE_OUTPUT: {
			_ShowView(fOutputTabView, fOutputTabView->IsHidden(), MSG_SHOW_HIDE_OUTPUT);
			break;
		}
		case MSG_TEXT_OVERWRITE: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor)  {
				editor->OverwriteToggle();
			}
			break;
		}

		case MSG_TOGGLE_TOOLBAR: {
			_ShowView(fToolBar, fToolBar->IsHidden(), MSG_TOGGLE_TOOLBAR);
			break;
		}
		case MSG_WHITE_SPACES_TOGGLE: {
			GenioNames::Settings.show_white_space = !GenioNames::Settings.show_white_space;
			for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
				Editor* editor = fTabManager->EditorAt(index);
				editor->ShowWhiteSpaces(GenioNames::Settings.show_white_space);
			}
			ActionManager::SetPressed(MSG_WHITE_SPACES_TOGGLE, GenioNames::Settings.show_white_space);
			break;
		}
		case MSG_WINDOW_SETTINGS: {
			SettingsWindow *window = new SettingsWindow();
			window->Show();

			break;
		}
		case TABMANAGER_TAB_SELECTED: {
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				Editor* editor = fTabManager->EditorAt(index);
				// TODO notify and check index too
				if (editor == nullptr) {
					LogError("Selecting editor but it's null! (index %d)", index);
					break;
				}

				editor->GrabFocus();
				// In multifile open not-focused files place scroll just after
				// caret line when reselected. Ensure visibility.
				// TODO caret policy
				editor->EnsureVisiblePolicy();
				_UpdateTabChange(editor, "TABMANAGER_TAB_SELECTED");
			}
			break;
		}
		case TABMANAGER_TAB_CLOSE_MULTI: {
			_CloseMultipleTabs(message);
			break;
		}
		case TABMANAGER_TAB_NEW_OPENED: {
			int32 index;
			int32 be_line = message->GetInt32("be:line",  -1);
			int32 lsp_char = message->GetInt32("lsp:character", -1);

			if (message->FindInt32("index", &index) == B_OK) {
				Editor* editor = fTabManager->EditorAt(index);

				if (lsp_char >= 0 && be_line > 0)
					editor->GoToLSPPosition(be_line - 1, lsp_char);
				else
				if (be_line > 0)
					editor->GoToLine(be_line);
				else
					editor->SetSavedCaretPosition();
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
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
	if(unsavedIndex.empty())
		return true;

	std::vector<std::string> unsavedPaths;
	for(int i:unsavedIndex) {
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

		int32 choice = alert->Go();

		if (choice == 0)
			return B_ERROR;
		else if (choice == 2) {
			_FileSave(unsavedIndex[0]);
		}
		return true;
	}
	//Let's use Koder QuitAlert!


	QuitAlert* quitAlert = new QuitAlert(unsavedPaths);
	auto filesToSave = quitAlert->Go();

	if (filesToSave.empty()) //Cancel the request!
		return false;

	auto bter = filesToSave.begin();
	auto iter = unsavedIndex.begin();
	while(iter != unsavedIndex.end()) {
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

	// Files to reopen
	if (GenioNames::Settings.reopen_files == true) {
		TPreferences files(GenioNames::kSettingsFilesToReopen,
							GenioNames::kApplicationName, 'FIRE');
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
	}

	// remove link between all editors and all projects
	for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
		fTabManager->EditorAt(index)->SetProjectFolder(NULL);
	}

	// Projects to reopen
	if (GenioNames::Settings.reopen_projects == true) {

		TPreferences projects(GenioNames::kSettingsProjectsToReopen,
								GenioNames::kApplicationName, 'PRRE');

		projects.MakeEmpty();

		for (int32 index = 0; index < fProjectFolderObjectList->CountItems(); index++) {
			ProjectFolder *project = fProjectFolderObjectList->ItemAt(index);
			projects.AddString("project_to_reopen", project->Path());
			if (project->Active())
				projects.SetString("active_project", project->Path());
			// Avoiding leaks
			//TODO:---> _ProjectOutlineDepopulate(project);
			delete project;
		}
	}

	if(fGoToLineWindow != nullptr) {
		fGoToLineWindow->LockLooper();
		fGoToLineWindow->Quit();
	}


	GenioNames::Settings.find_wrap = (bool)fFindWrapCheck->Value();
	GenioNames::Settings.find_whole_word = (bool)fFindWholeWordCheck->Value();
	GenioNames::Settings.find_match_case = (bool)fFindCaseSensitiveCheck->Value();
	GenioNames::Settings.show_projects = ActionManager::IsPressed(MSG_SHOW_HIDE_PROJECTS);
	GenioNames::Settings.show_output   = ActionManager::IsPressed(MSG_SHOW_HIDE_OUTPUT);
	GenioNames::Settings.show_toolbar  = ActionManager::IsPressed(MSG_TOGGLE_TOOLBAR);


	GenioNames::SaveSettingsVars();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


status_t
GenioWindow::_AddEditorTab(entry_ref* ref, int32 index, int32 be_line, int32 lsp_char)
{
	// Check existence
	BEntry entry(ref);

	if (entry.Exists() == false)
		return B_ERROR;

	Editor* editor = new Editor(ref, BMessenger(this));

	if (editor == nullptr)
		return B_ERROR;


	fTabManager->AddTab(editor, ref->name, index, be_line, lsp_char);



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
	if (fActiveProject->SaveOnBuild())
		_FileSaveAll(fActiveProject);

	fIsBuilding = true;
	fProjectsFolderBrowser->SetBuildingPhase(fIsBuilding);
	_UpdateProjectActivation(false);

	fBuildLogView->Clear();
	_ShowLog(kBuildLog);

	LogInfoF("Build started: [%s]", fActiveProject->Name().String());

	BMessage message;
	message.AddString("cmd", command);
	message.AddString("cmd_type", "build");

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
	fProjectsFolderBrowser->SetBuildingPhase(fIsBuilding);

	BMessage message;
	message.AddString("cmd", command);
	message.AddString("cmd_type", "clean");

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
	if (fActiveProject->GetBuildMode() == BuildMode::ReleaseMode)
		return B_ERROR;

	// attempt to launch Debugger with BRoster::Launch() failed so we use a more traditional
	// approach here
	BString commandLine;
	commandLine.SetToFormat("Debugger %s %s",
							EscapeQuotesWrap(fActiveProject->GetTarget()).String(),
							EscapeQuotesWrap(fActiveProject->GetExecuteArgs()).String());
	return system(commandLine) == 0 ? B_OK : errno;
}


status_t
GenioWindow::_RemoveTab(int32 index)
{
	// Should not happen
	if (index < 0) {
		LogErrorF("No file selected %d", index);
		return B_ERROR;
	}
	Editor* editor = fTabManager->EditorAt(index);
	fTabManager->RemoveTab(index);
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

status_t
GenioWindow::_FileOpen(BMessage* msg)
{
	entry_ref ref;
	status_t status = B_OK;
	int32 refsCount = 0;
	int32 openedIndex;
	int32 nextIndex;
	BString notification;


	// If user choose to reopen files reopen right index
	// otherwise use default behaviour (see below)
	if (msg->FindInt32("opened_index", &nextIndex) != B_OK)
		nextIndex = fTabManager->CountTabs();

	const int32 be_line   = msg->GetInt32("be:line", -1);
	const int32 lsp_char	= msg->GetInt32("lsp:character", -1);

	bool openWithPreferred	= msg->GetBool("openWithPreferred", false);

	while (msg->FindRef("refs", refsCount++, &ref) == B_OK) {

		if (!_FileIsSupported(&ref)) {
			if (openWithPreferred)
				_FileOpenWithPreferredApp(&ref); //TODO make this optional?
			continue;
		}	else {
			be_roster->AddToRecentDocuments(&ref, GenioNames::GetSignature());
		}

		// Do not reopen an already opened file
		if ((openedIndex = _GetEditorIndex(&ref)) != -1) {

			if (openedIndex != fTabManager->SelectedTabIndex()) {
				fTabManager->SelectTab(openedIndex);
			}

			if (lsp_char >= 0 && be_line > -1) {
				fTabManager->EditorAt(openedIndex)->GoToLSPPosition(be_line - 1, lsp_char);
			}
			else if (be_line > -1) {
				fTabManager->EditorAt(openedIndex)->GoToLine(be_line);
			}

			fTabManager->SelectedEditor()->GrabFocus();
			continue;
		}

		int32 index = fTabManager->CountTabs();

		if (_AddEditorTab(&ref, index, be_line, lsp_char) != B_OK)
			continue;

		assert(index >= 0);

		Editor* editor = fTabManager->EditorAt(index);

		if (editor == nullptr) {
			notification << ref.name << ": NULL editor pointer";
			_SendNotification(notification, "FILE_ERR");
			return B_ERROR;
		}

		/*
			here we try to assign the right "LSPClientWrapper" to the Editor..
		*/

		// Check if already open
		BString baseDir("");
		for (int32 index = 0; index < fProjectFolderObjectList->CountItems(); index++) {
			ProjectFolder * project = fProjectFolderObjectList->ItemAt(index);
			BString projectPath = project->Path();
			projectPath = projectPath.Append("/");
			if (editor->FilePath().StartsWith(projectPath)) {
				editor->SetProjectFolder(project);
			}
		}

		status = editor->LoadFromFile();


		if (status != B_OK) {
			continue;
		}

		editor->ApplySettings();
		editor->SetZoom(GenioNames::Settings.editor_zoom);
		editor->ShowLineEndings(GenioNames::Settings.show_line_endings);
		editor->ShowWhiteSpaces(GenioNames::Settings.show_white_space);

		// First tab gets selected by tabview
		if (index > 0)
			fTabManager->SelectTab(index);

		notification << "File open: " << editor->Name()
			<< " [" << fTabManager->CountTabs() - 1 << "]";
		_SendNotification(notification, "FILE_OPEN");
		notification.SetTo("");

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


	std::string fileType = Genio::file_type(BPath(ref).Path());

	if (fileType != "")
		return true;

	BNodeInfo info(&entry);

	if (info.InitCheck() == B_OK) {
		char mime[B_MIME_TYPE_LENGTH + 1];
		if (info.GetType(mime) != B_OK) {
			LogError("Error in getting mime type from file [%s]", BPath(ref).Path());
			mime[0]='\0';
		}
		if (strlen(mime) == 0 || strcmp(mime, "application/octet-stream") == 0)
		{
			if (update_mime_info(BPath(ref).Path(), false, true, B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL) == B_OK)
			{
				if (info.GetType(mime) != B_OK) {
					LogError("Error in getting mime type from file [%s]", BPath(ref).Path());
					mime[0]='\0';
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
       if (entry.InitCheck() != B_OK || entry.IsDirectory())
               return false;

       BString commandLine;
       commandLine.SetToFormat("/bin/open \"%s\"", BPath(ref).Path());
       return system(commandLine) == 0 ? B_OK : errno;
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
		LogInfoF("File saved! (%s) bytes(%ld) -> written(%ld)", editor->FilePath(), length, written);
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
			_SendNotification(notification, "FILE_ERR");
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
		_SendNotification(notification, "FILE_ERR");
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
	if (GenioNames::Settings.trim_trailing_whitespace)
		editor->TrimTrailingWhitespace();
}


void
GenioWindow::_PostFileSave(Editor* editor)
{
	// TODO: Also handle cases where the file is saved from outside Genio ?
	ProjectFolder* project = editor->GetProjectFolder();
	if (project != nullptr && project->BuildOnSave()) {
		if (!fIsBuilding)
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

	  BString grepCommand("grep -IHrn");
	  grepCommand += extraParameters;
	  grepCommand += " -- ";
	  grepCommand += EscapeQuotesWrap(text);
	  grepCommand += " ";
	  grepCommand += EscapeQuotesWrap(fActiveProject->Path());

	 fSearchResultPanel->StartSearch(grepCommand, fActiveProject->Path());
}

int32
GenioWindow::_GetEditorIndex(entry_ref* ref, bool checkExists)
{
	BEntry entry(ref, true);
	int32 filesCount = fTabManager->CountTabs();

	// Could try to reopen at start a saved index that was deleted,
	// check existence
	if (checkExists && entry.Exists() == false)
		return -1;

	for (int32 index = 0; index < filesCount; index++) {

		Editor* editor = fTabManager->EditorAt(index);

		if (editor == nullptr) {
			BString notification;
			notification
				<< "Index " << index << ": NULL editor pointer";
			_SendNotification(notification, "FILE_ERR");
			continue;
		}

		BEntry matchEntry(editor->FileRef(), true);

		if (matchEntry == entry)
			return index;
	}
	return -1;
}

int32
GenioWindow::_GetEditorIndex(node_ref* nref)
{
	int32 filesCount = fTabManager->CountTabs();

	for (int32 index = 0; index < filesCount; index++) {

		Editor* editor = fTabManager->EditorAt(index);

		if (editor == nullptr) {
			BString notification;
			notification
				<< "Index " << index << ": NULL editor pointer";
			_SendNotification(notification, "FILE_ERR");
			continue;
		}

		if (*nref == *editor->NodeRef())
			return index;
	}
	return -1;
}


void
GenioWindow::_GetFocusAndSelection(BTextControl* control)
{
	control->MakeFocus(true);
	// If some text is selected, use that TODO index check
	Editor* editor = fTabManager->SelectedEditor();
	if (editor && editor->IsTextSelected()) {
		int32 size = editor->SendMessage(SCI_GETSELTEXT, 0, 0);
		char text[size + 1];
		editor->SendMessage(SCI_GETSELTEXT, 0, (sptr_t)text);
		control->SetText(text);
	}
	else
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

	int32 index = _GetEditorIndex(oldRef, false);

 	int32 choice = alert->Go();

	if (choice == 0)
		return;
	else if (choice == 1) {
		_FileRequestClose(index);
	}
	else if (choice == 2) {
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
			for (int32 index = 0; index < fProjectFolderObjectList->CountItems(); index++) {
				ProjectFolder * project = fProjectFolderObjectList->ItemAt(index);
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
		_SendNotification(notification, "FILE_INFO");
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
		_SendNotification(notification, "FILE_INFO");
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


	fFindCaseSensitiveCheck = new BCheckBox(B_TRANSLATE("Match case"));
	fFindWholeWordCheck = new BCheckBox(B_TRANSLATE("Whole word"));
	fFindWrapCheck = new BCheckBox(B_TRANSLATE("Wrap"));

	fFindWrapCheck->SetValue((int32)GenioNames::Settings.find_wrap);
	fFindWholeWordCheck->SetValue((int32)GenioNames::Settings.find_whole_word);
	fFindCaseSensitiveCheck->SetValue((int32)GenioNames::Settings.find_match_case);


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

	fFindGroup->AddAction(MSG_FIND_MARK_ALL, B_TRANSLATE("Bookmark all"), "kIconBookmarkPen");
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
	fRunConsoleProgramText = new BTextControl("ReplaceTextControl", "", "", nullptr);
	fRunConsoleProgramButton = new BButton("RunConsoleProgramButton",
		B_TRANSLATE("Run"), new BMessage(MSG_RUN_CONSOLE_PROGRAM));

	BString tooltip("cwd: ");
	tooltip << GenioNames::Settings.projects_directory;
	fRunConsoleProgramText->SetToolTip(tooltip);

	fRunConsoleProgramGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0f)
		.Add(BLayoutBuilder::Group<>(B_HORIZONTAL, 0.0f)
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
								   B_TRANSLATE("Open"),
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
								   "App_OpenTargetFolder");
	ActionManager::RegisterAction(MSG_WHITE_SPACES_TOGGLE,
								   B_TRANSLATE("Show white spaces"),
								   "", "");
	ActionManager::RegisterAction(MSG_LINE_ENDINGS_TOGGLE,
								   B_TRANSLATE("Show line endings"),
								   "", "");

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
								   B_TRANSLATE("Open project"),
								   "","",'O', B_OPTION_KEY);

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
}

void
GenioWindow::_InitMenu()
{
	// Menu
	fMenuBar = new BMenuBar("menubar");

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

	fileMenu->AddSeparatorItem();
	ActionManager::AddItem(B_QUIT_REQUESTED, fileMenu);

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

	ActionManager::SetEnabled(MSG_FIND_GROUP_SHOW, false);
	ActionManager::SetEnabled(MSG_REPLACE_GROUP_SHOW, false);
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
	projectMenu->AddItem(fMakeCatkeysItem = new BMenuItem ("Make catkeys",
		new BMessage(MSG_MAKE_CATKEYS)));
	projectMenu->AddItem(fMakeBindcatalogsItem = new BMenuItem ("Make bindcatalogs",
		new BMessage(MSG_MAKE_BINDCATALOGS)));

	ActionManager::SetEnabled(MSG_BUILD_PROJECT, false);
	ActionManager::SetEnabled(MSG_CLEAN_PROJECT, false);
	ActionManager::SetEnabled(MSG_RUN_TARGET, false);

	fBuildModeItem->SetEnabled(false);
	ActionManager::SetEnabled(MSG_DEBUG_PROJECT, false);
	fMakeCatkeysItem->SetEnabled(false);
	fMakeBindcatalogsItem->SetEnabled(false);

	projectMenu->AddSeparatorItem();
	projectMenu->AddItem(new BMenuItem(B_TRANSLATE("Project settings" B_UTF8_ELLIPSIS),
		new BMessage(MSG_PROJECT_SETTINGS)));

	fMenuBar->AddItem(projectMenu);

	fGitMenu = new BMenu(B_TRANSLATE("Git"));
	fGitMenu->AddItem(fGitBranchItem = new BMenuItem(B_TRANSLATE_COMMENT("Branch",
		"The git command"), nullptr));
	BMessage* git_branch_message = new BMessage(MSG_GIT_COMMAND);
	git_branch_message->AddString("command", "branch");
	fGitBranchItem->SetMessage(git_branch_message);

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
	windowMenu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(MSG_WINDOW_SETTINGS), 'P', B_OPTION_KEY));
	fMenuBar->AddItem(windowMenu);

	BMenu* helpMenu = new BMenu(B_TRANSLATE("Help"));
	helpMenu->AddItem(new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED)));

	fMenuBar->AddItem(helpMenu);
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


	ActionManager::SetEnabled(MSG_FIND_GROUP_TOGGLED, false);
	ActionManager::SetEnabled(MSG_REPLACE_GROUP_TOGGLED, false);
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

	// Project list
	fProjectFolderObjectList = new BObjectList<ProjectFolder>();
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
	TPreferences prefs(GenioNames::kSettingsFileName,
		GenioNames::kApplicationName, 'PRSE');

	BEntry entry(prefs.GetString("projects_directory"), true);

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
		project->Active(true);
		_UpdateProjectActivation(true);
	}
	else {
		// There was an active project already
		fActiveProject->Active(false);
		fActiveProject = project;
		project->Active(true);
		_UpdateProjectActivation(true);
	}

	// Update run command working directory tooltip too
	BString tooltip;
	tooltip << "cwd: " << fActiveProject->Path();
	fRunConsoleProgramText->SetToolTip(tooltip);
}

void
GenioWindow::_ProjectFileDelete()
{
	entry_ref ref;
	int32 openedIndex;
	BEntry entry(fProjectsFolderBrowser->GetCurrentProjectFileFullPath());
	entry.GetRef(&ref);
	char name[B_FILE_NAME_LENGTH];

	if (!entry.Exists())
		return;

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
		if (entry.IsFile())
			if ((openedIndex = _GetEditorIndex(&ref)) != -1)
				_RemoveTab(openedIndex);

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
	ProjectItem *item = fProjectsFolderBrowser->GetCurrentProjectItem();
	fProjectsFolderBrowser->InitRename(item);
}



status_t
GenioWindow::_ProjectRemoveDir(const BString& dirPath)
{
	BDirectory dir(dirPath);
	BEntry startEntry(dirPath);
	BEntry entry;

	while (dir.GetNextEntry(&entry) == B_OK) {

		if (entry.IsDirectory()) {
			BDirectory newdir(&entry);
			if (newdir.CountEntries() == 0) {
				// Empty dir, remove
				entry.Remove();
			} else {
				// Populated dir, recurse
				BPath newPath(dirPath);
				newPath.Append(entry.Name());

				_ProjectRemoveDir(newPath.Path());
			}
		} else {
			// It is a file, remove
			entry.Remove();
		}
	}

	return startEntry.Remove();
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

	if(!_FileRequestSaveList(unsavedFiles))
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
		tooltip << "cwd: " << GenioNames::Settings.projects_directory;
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
	fProjectFolderObjectList->RemoveItem(project);

	project->Close();

	delete project;

	// Select a new active project
	if (wasActive) {
		ProjectItem* item = dynamic_cast<ProjectItem*>(fProjectsFolderBrowser->FullListItemAt(0));
		if (item != nullptr)
			_ProjectFolderActivate((ProjectFolder*)item->GetSourceItem());
	}
	BString notification;
	notification << closed << " "  << name;
	_SendNotification(notification, "PROJ_CLOSE");

}

void
GenioWindow::_ProjectFolderOpen(BMessage *message)
{
	entry_ref ref;

	status_t status = message->FindRef("refs", &ref);
	if (status != B_OK)
		throw BException(B_TRANSLATE("Invalid project folder"),0,status);

	BPath path(&ref);

	_ProjectFolderOpen(path.Path());
}

void
GenioWindow::_ProjectFolderOpen(const BString& folder, bool activate)
{
	BPath path(folder);

	ProjectFolder* newProject = new ProjectFolder(path.Path());

	// Check if already open
	for (int32 index = 0; index < fProjectFolderObjectList->CountItems(); index++) {
		ProjectFolder * pProject =(ProjectFolder*)fProjectFolderObjectList->ItemAt(index);
		if (pProject->Path() == newProject->Path()) {
			delete newProject;
			return;
		}
	}

	if (newProject->Open() != B_OK) {
		BString notification;
		notification << "Project open fail: " << newProject->Name();
		_SendNotification( notification.String(), "PROJ_OPEN_FAIL");
		delete newProject;

		return;
	}

	fProjectsFolderBrowser->ProjectFolderPopulate(newProject);
	fProjectFolderObjectList->AddItem(newProject);

	BString opened("Project open: ");
	if (fProjectFolderObjectList->CountItems() == 1 || activate == true) {
		_ProjectFolderActivate(newProject);
		opened = "Active project open: ";
	}

	BString notification;
	notification << opened << newProject->Name() << " at " << newProject->Path();
	_SendNotification(notification, "PROJ_OPEN");

	//let's check if any open editor is related to this project
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
}



// TODO: _OpenTerminalWorkingDirectory(), _ShowCurrentItemInTracker() and
// _FileOpenWithPreferredApp(const entry_ref* ref) share almost the same code
// They should be refactored to use a common base method e.g. _OpenWith()
status_t
GenioWindow::_OpenTerminalWorkingDirectory()
{
	BString commandLine, itemPath, notification;
	status_t returnStatus;

	int32 selection = fProjectsFolderBrowser->CurrentSelection();
	if (selection < 0)
		return B_BAD_VALUE;
	ProjectItem* selectedProjectItem = fProjectsFolderBrowser->GetCurrentProjectItem();
	itemPath = selectedProjectItem->GetSourceItem()->Path();

	commandLine.SetToFormat("Terminal -w %s &", EscapeQuotesWrap(itemPath.String()).String());

	returnStatus = system(commandLine);
	if (returnStatus != B_OK) {
		notification <<
			"An error occurred while opening Terminal and setting working directory to: ";
	} else {
		notification <<
			"Terminal successfully opened with working directory: ";
	}
	notification << itemPath;
	_SendNotification(notification, "PROJ_TERM");
	return returnStatus == 0 ? B_OK : errno;
}


status_t
GenioWindow::_ShowCurrentItemInTracker()
{
	BString commandLine, itemPath, notification;
	int returnStatus = -1;

	int32 selection = fProjectsFolderBrowser->CurrentSelection();
	if (selection < 0)
		return B_BAD_VALUE;
	ProjectItem* selectedProjectItem = fProjectsFolderBrowser->GetCurrentProjectItem();
	itemPath = selectedProjectItem->GetSourceItem()->Path();

	BEntry itemEntry(itemPath);
	BEntry parentDirectory;

	if (itemEntry.GetParent(&parentDirectory) == B_OK) {
		BPath directoryPath;
		if (parentDirectory.GetPath(&directoryPath) == B_OK) {
			commandLine.SetToFormat("/bin/open %s", EscapeQuotesWrap(directoryPath.Path()).String());
			returnStatus = system(commandLine);
		} else {
			notification << "An error occurred when showing an item in Tracker: " << directoryPath.Path();
			_SendNotification(notification, "PROJ_SHOW");
		}
	} else {
		notification << "An error occurred while retrieving parent directory of " << itemPath;
		_SendNotification(notification, "PROJ_TRACK");
	}
	return returnStatus == 0 ? B_OK : errno;
}

status_t
GenioWindow::_ShowInTracker(entry_ref *ref)
{
	status_t status = 0;
	BEntry itemEntry(ref);
	BPath path;
	BString commandLine;
	if (itemEntry.GetPath(&path) == B_OK) {
		commandLine.SetToFormat("/bin/open %s", EscapeQuotesWrap(path.Path()).String());
		status = system(commandLine);
	} else {
		OKAlert("Open in Tracker", B_TRANSLATE("Could not open template folder in Tracker"), B_WARNING_ALERT);
	}
	return status == 0 ? B_OK : errno;
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
//			retValue = editor->ReplaceAndFindPrevious(selection, replacement, flags, wrap);
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
GenioWindow::_ReplaceAllow()
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
		chdir(GenioNames::Settings.projects_directory);
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

			command << fActiveProject->GetTarget();
			if (!args.IsEmpty())
				command << " " << args;
			// TODO: Go to appropriate directory
			// chdir(...);
		// }

		BMessage message;
		message.AddString("cmd", command);
		message.AddString("cmd_type", "run");

		fConsoleIOView->MakeFocus(true);

		fConsoleIOView->RunCommand(&message);

	} else {
	// TODO: run args
		entry_ref ref;
		entry.SetTo(fActiveProject->GetTarget());
		entry.GetRef(&ref);
		be_roster->Launch(&ref, 1, NULL);
	}
}

void
GenioWindow::_SendNotification(BString message, BString type)
{
	if (GenioNames::Settings.enable_notifications == false)
		return;

	LogInfo("Notification: %s - %s", type.String(), message.String());
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
		if (fActiveProject->Git())
			fGitMenu->SetEnabled(true);
		else
			fGitMenu->SetEnabled(false);

		// Build mode
		bool releaseMode = (fActiveProject->GetBuildMode() == BuildMode::ReleaseMode);

		fDebugModeItem->SetMarked(!releaseMode);
		fReleaseModeItem->SetMarked(releaseMode);

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
void
GenioWindow::_UpdateSavepointChange(int32 index, const BString& caller)
{
	assert (index > -1 && index < fTabManager->CountTabs());

	Editor* editor = fTabManager->EditorAt(index);

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
}

// Updating menu, toolbar, title.
// Also cleaning Status bar if no open files
void
GenioWindow::_UpdateTabChange(Editor* editor, const BString& caller)
{
	// All files are closed
	if (editor == nullptr) {
		// ToolBar Items
		_FindGroupShow(false);
		ActionManager::SetEnabled(MSG_FILE_FOLD_TOGGLE, false);
		ActionManager::SetEnabled(B_UNDO, false);
		ActionManager::SetEnabled(B_REDO, false);

		ActionManager::SetEnabled(MSG_FILE_SAVE, false);
		ActionManager::SetEnabled(MSG_FILE_SAVE_AS, false);
		ActionManager::SetEnabled(MSG_FILE_SAVE_ALL, false);
		ActionManager::SetEnabled(MSG_FILE_CLOSE, false);
		ActionManager::SetEnabled(MSG_FILE_CLOSE_ALL, false);

		ActionManager::SetEnabled(MSG_BUFFER_LOCK, false);
		ActionManager::SetEnabled(MSG_FIND_PREVIOUS, false);
		ActionManager::SetEnabled(MSG_FIND_NEXT, false);
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
		ActionManager::SetEnabled(MSG_FIND_GROUP_TOGGLED, false);
		ActionManager::SetEnabled(MSG_REPLACE_GROUP_TOGGLED, false);
		ActionManager::SetEnabled(MSG_FIND_GROUP_SHOW, false);
		ActionManager::SetEnabled(MSG_REPLACE_GROUP_SHOW, false);
		ActionManager::SetEnabled(MSG_FIND_NEXT, false);
		ActionManager::SetEnabled(MSG_FIND_PREVIOUS, false);
		ActionManager::SetEnabled(MSG_GOTO_LINE, false);
		fBookmarksMenu->SetEnabled(false);

		if (GenioNames::Settings.fullpath_title == true)
			SetTitle(GenioNames::kApplicationName);

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

	ActionManager::SetEnabled(MSG_DUPLICATE_LINE, !editor->IsReadOnly());
	ActionManager::SetEnabled(MSG_DELETE_LINES, !editor->IsReadOnly());
	ActionManager::SetEnabled(MSG_COMMENT_SELECTED_LINES, !editor->IsReadOnly());
	ActionManager::SetEnabled(MSG_FILE_TRIM_TRAILING_SPACE, !editor->IsReadOnly());

	ActionManager::SetEnabled(MSG_AUTOCOMPLETION, !editor->IsReadOnly() && editor->GetProjectFolder());
	ActionManager::SetEnabled(MSG_FORMAT, !editor->IsReadOnly() && editor->GetProjectFolder());
	ActionManager::SetEnabled(MSG_GOTODEFINITION, editor->GetProjectFolder());
	ActionManager::SetEnabled(MSG_GOTODECLARATION, editor->GetProjectFolder());
	ActionManager::SetEnabled(MSG_GOTOIMPLEMENTATION, editor->GetProjectFolder());
	ActionManager::SetEnabled(MSG_SWITCHSOURCE, (Genio::file_type(editor->Name().String()).compare("c++") == 0));

	ActionManager::SetEnabled(MSG_FIND_GROUP_TOGGLED, true);
	ActionManager::SetEnabled(MSG_REPLACE_GROUP_TOGGLED, true);
	ActionManager::SetEnabled(MSG_FIND_GROUP_SHOW, true);
	ActionManager::SetEnabled(MSG_REPLACE_GROUP_SHOW, true);
	ActionManager::SetEnabled(MSG_FIND_NEXT, true);
	ActionManager::SetEnabled(MSG_FIND_PREVIOUS, true);
	ActionManager::SetEnabled(MSG_GOTO_LINE, true);

	fBookmarksMenu->SetEnabled(true);

	// File full path in window title
	if (GenioNames::Settings.fullpath_title == true) {
		BString title;
		title << GenioNames::kApplicationName << ": " << editor->FilePath();
		SetTitle(title.String());
	}

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
GenioWindow::UpdateMenu()
{
	ProjectItem *item = fProjectsFolderBrowser->GetCurrentProjectItem();
	if (item != nullptr) {
		if (item->GetSourceItem()->Type() != SourceItemType::FileItem)
			fFileNewMenuItem->SetViewMode(TemplatesMenu::ViewMode::SHOW_ALL_VIEW_MODE);
		else
			fFileNewMenuItem->SetViewMode(TemplatesMenu::ViewMode::DISABLE_FILES_VIEW_MODE, false);
	}
}
