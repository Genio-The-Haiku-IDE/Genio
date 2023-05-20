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
#include "TPreferences.h"
#include "TextUtils.h"
#include "Utils.h"
#include "EditorKeyDownMessageFilter.h"
#include "TextControlFloater.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioWindow"


constexpr auto kRecentFilesNumber = 14 + 1;

// If enabled check menu open point
//static const auto kToolBarSize = 29;

static constexpr float kTabBarHeight = 30.0f;


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
	, fFileNewMenuItem(nullptr)
	, fSaveMenuItem(nullptr)
	, fSaveAsMenuItem(nullptr)
	, fSaveAllMenuItem(nullptr)
	, fCloseMenuItem(nullptr)
	, fCloseAllMenuItem(nullptr)
	, fFoldMenuItem(nullptr)
	, fUndoMenuItem(nullptr)
	, fRedoMenuItem(nullptr)
	, fCutMenuItem(nullptr)
	, fCopyMenuItem(nullptr)
	, fPasteMenuItem(nullptr)
	, fSelectAllMenuItem(nullptr)
	, fOverwiteItem(nullptr)
	, fToggleWhiteSpacesItem(nullptr)
	, fToggleLineEndingsItem(nullptr)
	, fDeleteLinesItem(nullptr)
	, fCommentSelectionItem(nullptr)
	, fLineEndingsMenu(nullptr)
	, fFindItem(nullptr)
	, fReplaceItem(nullptr)
	, fGoToLineItem(nullptr)
	, fBookmarksMenu(nullptr)
	, fBookmarkToggleItem(nullptr)
	, fBookmarkClearAllItem(nullptr)
	, fBookmarkGoToNextItem(nullptr)
	, fBookmarkGoToPreviousItem(nullptr)
	, fBuildItem(nullptr)
	, fCleanItem(nullptr)
	, fRunItem(nullptr)
	, fBuildModeItem(nullptr)
	, fReleaseModeItem(nullptr)
	, fDebugModeItem(nullptr)
	, fCargoMenu(nullptr)
	, fCargoUpdateItem(nullptr)
	, fDebugItem(nullptr)
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
	, fHgMenu(nullptr)
	, fHgStatusItem(nullptr)
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
{
	_InitMenu();

	_InitWindow();

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

	if (GenioNames::Settings.show_projects == false)
		fProjectsTabView->Hide();
	if (GenioNames::Settings.show_output == false)
		fOutputTabView->Hide();
	if (GenioNames::Settings.show_toolbar == false)
		fToolBar->Hide();

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

GenioWindow::~GenioWindow()
{
	//delete fEditorObjectList;
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
					if (!fFindGroup->IsHidden())
						fFindGroup->Hide();
					if (!fReplaceGroup->IsHidden())
						fReplaceGroup->Hide();
			} else if (CurrentFocus() == fReplaceTextControl->TextView()) {
					if (!fReplaceGroup->IsHidden())
						fReplaceGroup->Hide();
					fFindTextControl->MakeFocus(true);
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
					fOutputTabView->TabAt(0)->SetLabel(fProblemsPanel->TabLabel());
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
		case CONSOLEIOTHREAD_ERROR:
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
			_SendNotification(B_TRANSLATE("Find next not found"),
													"FIND_MISS");
			break;
		}
		case EDITOR_FIND_PREV_MISS: {
			_SendNotification(B_TRANSLATE("Find previous not found"),
													"FIND_MISS");
			break;
		}
		case EDITOR_FIND_COUNT: {
			int32 count;
			BString text;
			if (message->FindString("text_to_find", &text) == B_OK
				&& message->FindInt32("count", &count) == B_OK) {

				BString notification;
				notification << "\"" << text << "\""
					<< " "
					<< B_TRANSLATE("occurrences found:")
					<< " " << count;

				_ShowLog(kNotificationLog);
				_SendNotification(notification, "FIND_COUNT");
			}
			break;
		}

		case EDITOR_REPLACE_ALL_COUNT: {
			int32 count;
			if (message->FindInt32("count", &count) == B_OK) {
				BString notification;
				notification << B_TRANSLATE("Replacements done:") << " " << count;

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
					_SendNotification(B_TRANSLATE("Next Bookmark not found"),
													"FIND_MISS");
			}

			break;
		}
		case MSG_BOOKMARK_GOTO_PREVIOUS: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				if (!editor->BookmarkGoToPrevious())
					_SendNotification(B_TRANSLATE("Previous Bookmark not found"),
													"FIND_MISS");
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
				fToolBar->SetActionEnabled(MSG_BUFFER_LOCK, !editor->IsReadOnly());
			}
			break;
		}
		case MSG_BUILD_MODE_DEBUG: {
			fToolBar->FindButton(MSG_BUILD_MODE)->SetToolTip(B_TRANSLATE("Build mode: Debug"));
			fActiveProject->SetBuildMode(BuildMode::DebugMode);
			// _MakefileSetBuildMode(false);
			_UpdateProjectActivation(fActiveProject != nullptr);
			break;
		}
		case MSG_BUILD_MODE_RELEASE: {
			fToolBar->FindButton(MSG_BUILD_MODE)->SetToolTip(B_TRANSLATE("Build mode: Release"));
			fActiveProject->SetBuildMode(BuildMode::ReleaseMode);
			// _MakefileSetBuildMode(true);
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
		case MSG_BUILD_MODE: {
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
		case MSG_EOL_SET_TO_UNIX: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->SetEndOfLine(SC_EOL_LF);
			}
			break;
		}
		case MSG_EOL_SET_TO_DOS: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->SetEndOfLine(SC_EOL_CRLF);
			}
			break;
		}
		case MSG_EOL_SET_TO_MAC: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->SetEndOfLine(SC_EOL_CR);
			}
			break;
		}
		case MSG_FILE_CLOSE:
			_FileClose(fTabManager->SelectedTabIndex());
			break;
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
			_FindGroupShow();
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
			_FindGroupToggled();
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
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->ToggleLineEndings();
			}
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
		case MSG_SIGNATUREHELP: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) 
				editor->SignatureHelp();			
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
		case MSG_FILE_NEW:
		case MSG_PROJECT_MENU_NEW_FILE: 
		{
			ProjectItem* item = fProjectsFolderBrowser->GetCurrentProjectItem();
			if (!item) {
				LogError("Can't find current item");
				OKAlert("Create new file", "Please select the root of a project or a folder where "
							"you want to create file", B_WARNING_ALERT);
				return;
			}
			// in theory this should not happen as the menu item is enabled only when a folder or
			// a project folder is selected
			if (item->GetSourceItem()->Type() != SourceItemType::FolderItem) {
				LogDebug("Invoking on a non directory (%s)", item->GetSourceItem()->Name().String());
				return;
			}
			BEntry entry(item->GetSourceItem()->Path());
			entry_ref ref;
			if (entry.GetRef(&ref) != B_OK) {
				LogError("Invalid entry_ref for [%s]", item->GetSourceItem()->Path().String());
				return;
			}
			
			_CreateNewFile(message, &entry);
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
			_ReplaceGroupShow();
			break;
		case MSG_REPLACE_ALL: {
			_Replace(REPLACE_ALL);
			break;
		}
		case MSG_REPLACE_GROUP_TOGGLED:
			_ReplaceGroupToggled();
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
			if (fProjectsTabView->IsHidden()) {
				fProjectsTabView->Show();
			} else {
				fProjectsTabView->Hide();
			}
			break;
		}
		case MSG_SHOW_HIDE_OUTPUT: {
			if (fOutputTabView->IsHidden()) {
				fOutputTabView->Show();
			} else {
				fOutputTabView->Hide();
			}
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
			if (fToolBar->IsHidden()) {
				fToolBar->Show();
			} else {
				fToolBar->Hide();
			}
			break;
		}
		case MSG_WHITE_SPACES_TOGGLE: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor)  {
				editor->ToggleWhiteSpaces();
			}
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
		case TABMANAGER_TAB_CLOSE: {
			int32 index;
			if (message->FindInt32("index", &index) == B_OK)
				_FileClose(index);

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


bool
GenioWindow::QuitRequested()
{
	// Is there any modified file?
	if (_FilesNeedSave()) {
		BAlert* alert = new BAlert("QuitAndSaveDialog",
	 		B_TRANSLATE("There are modified files, do you want to save changes before quitting?"),
 			B_TRANSLATE("Cancel"), B_TRANSLATE("Don't save"), B_TRANSLATE("Save"),
 			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
  
		alert->SetShortcut(0, B_ESCAPE);
		
		int32 choice = alert->Go();

		if (choice == 0)
			return false;
		else if (choice == 1) { 

		} else if (choice == 2) {
			_FileSaveAll();
		}
	}

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
GenioWindow::_BuildProject()
{
	// Should not happen
	if (fActiveProject == nullptr)
		return B_ERROR;

	fIsBuilding = true;
	fProjectsFolderBrowser->SetBuildingPhase(fIsBuilding);
	_UpdateProjectActivation(false);

	fBuildLogView->Clear();
	_ShowLog(kBuildLog);

	BString text;
	text << "Build started: "  << fActiveProject->Name();
	_SendNotification(text, "PROJ_BUILD");

	BString command;
	command	<< fActiveProject->GetBuildCommand();

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

	_UpdateProjectActivation(false);

	fBuildLogView->Clear();
	_ShowLog(kBuildLog);

	BString notification;
	notification << B_TRANSLATE("Clean started:")
		 << " " << fActiveProject->Name();
	_SendNotification(notification, "PROJ_BUILD");

	BString command;
	command << fActiveProject->GetCleanCommand();

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

	const char *argv[] = { fActiveProject->GetTarget().String(), NULL};
	return be_roster->Launch("application/x-vnd.Haiku-Debugger", 1,	argv);
}

/*
 * ignoreModifications: the file is modified but has been removed
 * 						externally and user choose to discard it.
 */
status_t
GenioWindow::_FileClose(int32 index, bool ignoreModifications /* = false */)
{
	BString notification;

	// Should not happen
	if (index < 0) {
		notification << (B_TRANSLATE("No file selected"));
		_SendNotification(notification, "FILE_ERR");
		return B_ERROR;
	}

	Editor* editor = fTabManager->EditorAt(index);

	if (editor == nullptr) {
		notification << B_TRANSLATE("NULL editor pointer");
		_SendNotification(notification, "FILE_ERR");
		return B_ERROR;
	}

	if (editor->IsModified() && ignoreModifications == false) {
		BString text(B_TRANSLATE("Save changes to file \"%file%\""));
		text.ReplaceAll("%file%", editor->Name());
		
		BAlert* alert = new BAlert("CloseAndSaveDialog", text,
 			B_TRANSLATE("Cancel"), B_TRANSLATE("Don't save"), B_TRANSLATE("Save"),
 			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
   			 
		alert->SetShortcut(0, B_ESCAPE);
		
		int32 choice = alert->Go();

		if (choice == 0)
			return B_ERROR;
		else if (choice == 2) {
			_FileSave(index);
		}
	}

	notification << B_TRANSLATE("File close:") << " " << editor->Name();
	_SendNotification(notification, "FILE_CLOSE");

	fTabManager->RemoveTab(index);
	delete editor;

	// Was it the last one?
	if (fTabManager->CountTabs() == 0)
		_UpdateTabChange(nullptr, "_FileClose");

	return B_OK;
}

void
GenioWindow::_FileCloseAll()
{
	int32 tabsCount = fTabManager->CountTabs();
	// If there is something to close
	if (tabsCount > 0) {
		// Don't lose time in changing selection on removal
		fTabManager->SelectTab(int32(0));

		for (int32 index = tabsCount - 1; index >= 0; index--) {
			fTabManager->CloseTab(index);
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
			notification << ref.name
				<< ": "
				<< B_TRANSLATE("NULL editor pointer");
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

		// First tab gets selected by tabview
		if (index > 0)
			fTabManager->SelectTab(index);

		notification << B_TRANSLATE("File open:")  << "  "
			<< editor->Name()
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
//	status_t status;
	BString notification;

	// Should not happen
	if (index < 0) {
		notification << (B_TRANSLATE("No file selected"));
		_SendNotification(notification, "FILE_ERR");
		return B_ERROR;
	}

	Editor* editor = fTabManager->EditorAt(index);

	if (editor == nullptr) {
		notification << (B_TRANSLATE("NULL editor pointer"));
		_SendNotification(notification, "FILE_ERR");
		return B_ERROR;
	}

	// Readonly file, should not happen
	if (editor->IsReadOnly()) {
		notification << (B_TRANSLATE("File is Read-only"));
		_SendNotification(notification, "FILE_ERR");
		return B_ERROR;
	}

	// File not modified, happens at file save as
/*	if (!editor->IsModified()) {
		notification << (B_TRANSLATE("File not modified"));
		_SendNotification(notification, "FILE_ERR");
		return B_ERROR;
	}
*/
	// Stop monitoring if needed
	status_t status = editor->StopMonitoring();
	if (status != B_OK)
		LogErrorF("Error in StopMonitoring node (%s) (%s)", editor->Name().String(), strerror(status));

	ssize_t written = editor->SaveToFile();
	ssize_t length = editor->SendMessage(SCI_GETLENGTH, 0, 0);

	// Restart monitoring
	status = editor->StartMonitoring();
	if (status != B_OK)
		LogErrorF("Error in StartMonitoring node (%s) (%s)", editor->Name().String(), strerror(status));

	notification << B_TRANSLATE("File save:")  << "  "
		<< editor->Name()
		<< "    "
		<< length
		<< " "
		<< B_TRANSLATE("bytes")
		<< " -> "
		<< written
		<< " "
		<< B_TRANSLATE("written");

	_SendNotification(notification, length == written ? "FILE_SAVE" : "FILE_ERR");

	return B_OK;
}

void
GenioWindow::_FileSaveAll()
{
	int32 filesCount = fTabManager->CountTabs();

	for (int32 index = 0; index < filesCount; index++) {

		Editor* editor = fTabManager->EditorAt(index);

		if (editor == nullptr) {
			BString notification;
			notification << B_TRANSLATE("Index") << " " << index
				<< ": " << B_TRANSLATE("NULL editor pointer");
			_SendNotification(notification, "FILE_ERR");
			continue;
		}

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
			<< B_TRANSLATE("Index") << " " << selection
			<< ": " << B_TRANSLATE("NULL editor pointer");
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

bool
GenioWindow::_FilesNeedSave()
{
	for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
		Editor* editor = fTabManager->EditorAt(index);
		if (editor->IsModified()) {
			return true;
		}
	}

	return false;
}

void
GenioWindow::_FindGroupShow()
{
	if (fFindGroup->IsHidden()) {
		fFindGroup->Show();
	}
	_GetFocusAndSelection(fFindTextControl);
}

void
GenioWindow::_FindGroupToggled()
{
	bool findHidden = fFindGroup->IsHidden();
	if (findHidden) {
		_FindGroupShow();
	} else {
		
		fFindGroup->Hide();
		
		if (!fReplaceGroup->IsHidden())
			fReplaceGroup->Hide();
			
		Editor* editor = fTabManager->SelectedEditor();
		if (editor) {
			editor->GrabFocus();
		}		
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

	  fConsoleIOView->Clear();
	  
	  fConsoleIOView->TextView()->ScrollTo(
		  0, fConsoleIOView->TextView()->Bounds().bottom);

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

	  _RunInConsole(grepCommand);
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
				<< B_TRANSLATE("Index") << " " << index
				<< ": " << B_TRANSLATE("NULL editor pointer");
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
				<< B_TRANSLATE("Index") << " " << index
				<< ": " << B_TRANSLATE("NULL editor pointer");
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
	text << B_TRANSLATE("File \"%file%\" was moved externally,")
		 << "\n"
		 << B_TRANSLATE("do You want to ignore, close or reload it?");

	text.ReplaceAll("%file%", oldRef->name);

	BAlert* alert = new BAlert("FileMoveDialog", text,
 		B_TRANSLATE("Ignore"), B_TRANSLATE("Close"), B_TRANSLATE("Reload"),
 		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

 	alert->SetShortcut(0, B_ESCAPE);

	int32 index = _GetEditorIndex(oldRef, false);

 	int32 choice = alert->Go();
 
 	if (choice == 0)
		return;
	else if (choice == 1)
		_FileClose(index);
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
		notification << B_TRANSLATE("File info:") << "  "
			<< oldPath.Path() << " "
			<< B_TRANSLATE("moved externally to")
			<< " " << newPath.Path();
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
	text 	<< B_TRANSLATE("File \"%file%\" was removed externally,")
			<< "\n"
			<< B_TRANSLATE("do You want to keep the file or discard it?")
			<< "\n"
			<< B_TRANSLATE("If kept and modified save it or it will be lost");

	text.ReplaceAll("%file%", fileName);

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
	}	
	else if (choice == 1) {

		_FileClose(index, true);

		BString notification;
		notification << B_TRANSLATE("File info:") << "  "
			<< fileName << " " << B_TRANSLATE("removed externally");
		_SendNotification(notification, "FILE_INFO");
	}
}

void
GenioWindow::_HandleExternalStatModification(int32 index)
{
	if (index < 0) {
		return; //TODO notify
	}

	Editor* editor = fTabManager->EditorAt(index);

	BString text;
	text << GenioNames::kApplicationName << ":\n";
	text << (B_TRANSLATE("File \"%file%\" was modified externally, reload it?"));
	text.ReplaceAll("%file%", editor->Name());

	BAlert* alert = new BAlert("FileReloadDialog", text,
 		B_TRANSLATE("Ignore"), B_TRANSLATE("Reload"), nullptr,
 		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

 	alert->SetShortcut(0, B_ESCAPE);

 	int32 choice = alert->Go();
 
 	if (choice == 0)
		return;
	else if (choice == 1) {
		editor->Reload();

		BString notification;
		notification << B_TRANSLATE("File info:") << "  "
			<< editor->Name() << " "
			<< B_TRANSLATE("modified externally");
		_SendNotification(notification, "FILE_INFO");
	}
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

			_HandleExternalRemoveModification(_GetEditorIndex(&nref));
			//entry_ref ref(device, dir, name);
			//_HandleExternalRemoveModification(_GetEditorIndex(&ref));
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

	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY, new BMessage(MSG_FIND_NEXT));
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY, new BMessage(MSG_FIND_PREVIOUS));


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
	fFindGroup->AddAction(MSG_FIND_NEXT, B_TRANSLATE("Find Next"), "kIconDown_3");
	fFindGroup->AddAction(MSG_FIND_PREVIOUS, B_TRANSLATE("Find previous"), "kIconUp_3");
	fFindGroup->AddView(BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
												.Add(fFindWrapCheck)
												.Add(fFindWholeWordCheck)
												.Add(fFindCaseSensitiveCheck).View());

	fFindGroup->AddAction(MSG_FIND_MARK_ALL, B_TRANSLATE("Mark all"), "kIconBookmarkPen");	
	fFindGroup->AddAction(MSG_FIND_IN_FILES, B_TRANSLATE("Find in files"), "kIconFindInFiles");
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
	fTabManager->TabGroup()->SetExplicitMaxSize(BSize(B_SIZE_UNSET, kTabBarHeight));

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
GenioWindow::_InitMenu()
{
	// Menu
	fMenuBar = new BMenuBar("menubar");

	BMenu* fileMenu = new BMenu(B_TRANSLATE("File"));
	fileMenu->AddItem(fFileNewMenuItem = new TemplatesMenu(this, B_TRANSLATE("New"),
			MSG_FILE_NEW));	
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(MSG_FILE_OPEN), 'O'));
	fileMenu->AddItem(new BMenuItem(BRecentFilesList::NewFileListMenu(
			B_TRANSLATE("Open recent" B_UTF8_ELLIPSIS), nullptr, nullptr, this,
			kRecentFilesNumber, true, nullptr, GenioNames::kApplicationSignature), nullptr));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(fSaveMenuItem = new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(MSG_FILE_SAVE), 'S'));
	fileMenu->AddItem(fSaveAsMenuItem = new BMenuItem(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS),
		new BMessage(MSG_FILE_SAVE_AS)));
	fileMenu->AddItem(fSaveAllMenuItem = new BMenuItem(B_TRANSLATE("Save all"),
		new BMessage(MSG_FILE_SAVE_ALL), 'S', B_SHIFT_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(fCloseMenuItem = new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(MSG_FILE_CLOSE), 'W'));
	fileMenu->AddItem(fCloseAllMenuItem = new BMenuItem(B_TRANSLATE("Close all"),
		new BMessage(MSG_FILE_CLOSE_ALL), 'W', B_SHIFT_KEY));

	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));

	fSaveMenuItem->SetEnabled(false);
	fSaveAsMenuItem->SetEnabled(false);
	fSaveAllMenuItem->SetEnabled(false);
	fCloseMenuItem->SetEnabled(false);
	fCloseAllMenuItem->SetEnabled(false);

	fMenuBar->AddItem(fileMenu);

	BMenu* editMenu = new BMenu(B_TRANSLATE("Edit"));
	editMenu->AddItem(fUndoMenuItem = new BMenuItem(B_TRANSLATE("Undo"),
		new BMessage(B_UNDO), 'Z'));
	editMenu->AddItem(fRedoMenuItem = new BMenuItem(B_TRANSLATE("Redo"),
		new BMessage(B_REDO), 'Z', B_SHIFT_KEY));
	editMenu->AddSeparatorItem();
	editMenu->AddItem(fCutMenuItem = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	editMenu->AddItem(fCopyMenuItem = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
	editMenu->AddItem(fPasteMenuItem = new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE), 'V'));
	editMenu->AddSeparatorItem();
	editMenu->AddItem(fSelectAllMenuItem = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));
	editMenu->AddSeparatorItem();
	editMenu->AddItem(fOverwiteItem = new BMenuItem(B_TRANSLATE("Overwrite"),
		new BMessage(MSG_TEXT_OVERWRITE), B_INSERT));

	editMenu->AddSeparatorItem();
	editMenu->AddItem(fFoldMenuItem = new BMenuItem(B_TRANSLATE("Fold/Unfold all"),
		new BMessage(MSG_FILE_FOLD_TOGGLE)));
	fFoldMenuItem->SetEnabled(false);

	editMenu->AddItem(fToggleWhiteSpacesItem = new BMenuItem(B_TRANSLATE("Toggle white spaces"),
		new BMessage(MSG_WHITE_SPACES_TOGGLE)));
	editMenu->AddItem(fToggleLineEndingsItem = new BMenuItem(B_TRANSLATE("Toggle line endings"),
		new BMessage(MSG_LINE_ENDINGS_TOGGLE)));
	editMenu->AddItem(fDuplicateLineItem = new BMenuItem(B_TRANSLATE("Duplicate current line"),
		new BMessage(MSG_DUPLICATE_LINE), 'K'));
	editMenu->AddItem(fDeleteLinesItem = new BMenuItem(B_TRANSLATE("Delete lines"),
		new BMessage(MSG_DELETE_LINES), 'D'));	
	editMenu->AddItem(fCommentSelectionItem = new BMenuItem(B_TRANSLATE("Comment selected lines"),
		new BMessage(MSG_COMMENT_SELECTED_LINES), 'C', B_SHIFT_KEY));
	
	editMenu->AddSeparatorItem();	

	editMenu->AddItem(new BMenuItem(B_TRANSLATE("Autocompletion"), new BMessage(MSG_AUTOCOMPLETION), B_SPACE));
	editMenu->AddItem(new BMenuItem(B_TRANSLATE("Format"), new BMessage(MSG_FORMAT)));
	editMenu->AddItem(new BMenuItem(B_TRANSLATE("Go to definition"), new BMessage(MSG_GOTODEFINITION), 'G'));
	editMenu->AddItem(new BMenuItem(B_TRANSLATE("Go to declaration"), new BMessage(MSG_GOTODECLARATION)));
	editMenu->AddItem(new BMenuItem(B_TRANSLATE("Go to implementation"), new BMessage(MSG_GOTOIMPLEMENTATION)));
	editMenu->AddItem(new BMenuItem(B_TRANSLATE("Switch Source Header"), new BMessage(MSG_SWITCHSOURCE), B_TAB));
	editMenu->AddItem(new BMenuItem(B_TRANSLATE("Signature Help"), new BMessage(MSG_SIGNATUREHELP), '?'));

	editMenu->AddSeparatorItem();
	fLineEndingsMenu = new BMenu(B_TRANSLATE("Line endings"));
	fLineEndingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Set to Unix"),
		new BMessage(MSG_EOL_SET_TO_UNIX)));
	fLineEndingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Set to Dos"),
		new BMessage(MSG_EOL_SET_TO_DOS)));
	fLineEndingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Set to Mac"),
		new BMessage(MSG_EOL_SET_TO_MAC)));
	fLineEndingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Convert to Unix"),
		new BMessage(MSG_EOL_CONVERT_TO_UNIX)));
	fLineEndingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Convert to Dos"),
		new BMessage(MSG_EOL_CONVERT_TO_DOS)));
	fLineEndingsMenu->AddItem(new BMenuItem(B_TRANSLATE("Convert to Mac"),
		new BMessage(MSG_EOL_CONVERT_TO_MAC)));

	fUndoMenuItem->SetEnabled(false);
	fRedoMenuItem->SetEnabled(false);
	fCutMenuItem->SetEnabled(false);
	fCopyMenuItem->SetEnabled(false);
	fPasteMenuItem->SetEnabled(false);
	fSelectAllMenuItem->SetEnabled(false);
	fOverwiteItem->SetEnabled(false);
	fToggleWhiteSpacesItem->SetEnabled(false);
	fToggleLineEndingsItem->SetEnabled(false);
	fDuplicateLineItem->SetEnabled(false);
	fDeleteLinesItem->SetEnabled(false);
	fCommentSelectionItem->SetEnabled(false);
	fLineEndingsMenu->SetEnabled(false);

	editMenu->AddItem(fLineEndingsMenu);
	fMenuBar->AddItem(editMenu);
	
	BMenu* viewMenu = new BMenu(B_TRANSLATE("View"));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Zoom In"), new BMessage(MSG_VIEW_ZOOMIN), '+'));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Zoom Out"), new BMessage(MSG_VIEW_ZOOMOUT), '-'));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Zoom Reset"), new BMessage(MSG_VIEW_ZOOMRESET), '0'));
	fMenuBar->AddItem(viewMenu);
	
	BMenu* searchMenu = new BMenu(B_TRANSLATE("Search"));
	searchMenu->AddItem(fFindItem = new BMenuItem(B_TRANSLATE("Find"),
		new BMessage(MSG_FIND_GROUP_SHOW), 'F'));
	searchMenu->AddItem(fReplaceItem = new BMenuItem(B_TRANSLATE("Replace"),
		new BMessage(MSG_REPLACE_GROUP_SHOW), 'R'));
	searchMenu->AddItem(fGoToLineItem = new BMenuItem(B_TRANSLATE("Go to line" B_UTF8_ELLIPSIS),
		new BMessage(MSG_GOTO_LINE), '<'));

	fBookmarksMenu = new BMenu(B_TRANSLATE("Bookmark"));
	fBookmarksMenu->AddItem(fBookmarkToggleItem = new BMenuItem(B_TRANSLATE("Toggle"),
		new BMessage(MSG_BOOKMARK_TOGGLE)));
	fBookmarksMenu->AddItem(fBookmarkClearAllItem = new BMenuItem(B_TRANSLATE("Clear all"),
		new BMessage(MSG_BOOKMARK_CLEAR_ALL)));
	fBookmarksMenu->AddItem(fBookmarkGoToNextItem = new BMenuItem(B_TRANSLATE("Go to next"),
		new BMessage(MSG_BOOKMARK_GOTO_NEXT), 'N', B_CONTROL_KEY));
	fBookmarksMenu->AddItem(fBookmarkGoToPreviousItem = new BMenuItem(B_TRANSLATE("Go to previous"),
		new BMessage(MSG_BOOKMARK_GOTO_PREVIOUS),'P', B_CONTROL_KEY));

	fFindItem->SetEnabled(false);
	fReplaceItem->SetEnabled(false);
	fGoToLineItem->SetEnabled(false);
	fBookmarksMenu->SetEnabled(false);

	searchMenu->AddItem(fBookmarksMenu);
	fMenuBar->AddItem(searchMenu);
	
	BMenu* projectMenu = new BMenu(B_TRANSLATE("Project"));

	projectMenu->AddItem(new BMenuItem(B_TRANSLATE("Open Project"),
		new BMessage(MSG_PROJECT_OPEN), 'O', B_OPTION_KEY));
	projectMenu->AddItem(new BMenuItem(B_TRANSLATE("Close Project"),
		new BMessage(MSG_PROJECT_CLOSE), 'C', B_OPTION_KEY));
	projectMenu->AddSeparatorItem();
	
	projectMenu->AddItem(fBuildItem = new BMenuItem (B_TRANSLATE("Build Project"),
		new BMessage(MSG_BUILD_PROJECT), 'B'));
	projectMenu->AddItem(fCleanItem = new BMenuItem (B_TRANSLATE("Clean Project"),
		new BMessage(MSG_CLEAN_PROJECT)));
	projectMenu->AddItem(fRunItem = new BMenuItem (B_TRANSLATE("Run target"),
		new BMessage(MSG_RUN_TARGET), 'R', B_SHIFT_KEY));
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

	projectMenu->AddItem(fDebugItem = new BMenuItem (B_TRANSLATE("Debug Project"),
		new BMessage(MSG_DEBUG_PROJECT)));
	projectMenu->AddSeparatorItem();
	projectMenu->AddItem(fMakeCatkeysItem = new BMenuItem ("make catkeys",
		new BMessage(MSG_MAKE_CATKEYS)));
	projectMenu->AddItem(fMakeBindcatalogsItem = new BMenuItem ("make bindcatalogs",
		new BMessage(MSG_MAKE_BINDCATALOGS)));

	fBuildItem->SetEnabled(false);
	fCleanItem->SetEnabled(false);
	fRunItem->SetEnabled(false);
	fBuildModeItem->SetEnabled(false);
	fDebugItem->SetEnabled(false);
	fMakeCatkeysItem->SetEnabled(false);
	fMakeBindcatalogsItem->SetEnabled(false);

	projectMenu->AddSeparatorItem();
	projectMenu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(MSG_PROJECT_SETTINGS)));

	fMenuBar->AddItem(projectMenu);

	fGitMenu = new BMenu(B_TRANSLATE("Git"));
	fGitMenu->AddItem(fGitBranchItem = new BMenuItem(B_TRANSLATE("Branch"), nullptr));
	BMessage* git_branch_message = new BMessage(MSG_GIT_COMMAND);
	git_branch_message->AddString("command", "branch");
	fGitBranchItem->SetMessage(git_branch_message);

	fGitMenu->AddItem(fGitLogItem = new BMenuItem(B_TRANSLATE("Log"), nullptr));
	BMessage* git_log_message = new BMessage(MSG_GIT_COMMAND);
	git_log_message->AddString("command", "log");
	fGitLogItem->SetMessage(git_log_message);

	fGitMenu->AddItem(fGitPullItem = new BMenuItem(B_TRANSLATE("Pull"), nullptr));
	BMessage* git_pull_message = new BMessage(MSG_GIT_COMMAND);
	git_pull_message->AddString("command", "pull");
	fGitPullItem->SetMessage(git_pull_message);

	fGitMenu->AddItem(fGitStatusItem = new BMenuItem(B_TRANSLATE("Status"), nullptr));
	BMessage* git_status_message = new BMessage(MSG_GIT_COMMAND);
	git_status_message->AddString("command", "status");
	fGitStatusItem->SetMessage(git_status_message);

	fGitMenu->AddItem(fGitShowConfigItem = new BMenuItem(B_TRANSLATE("Show Config"), nullptr));
	BMessage* git_config_message = new BMessage(MSG_GIT_COMMAND);
	git_config_message->AddString("command", "config --list");
	fGitShowConfigItem->SetMessage(git_config_message);

	fGitMenu->AddItem(fGitTagItem = new BMenuItem(B_TRANSLATE("Tag"), nullptr));
	BMessage* git_tag_message = new BMessage(MSG_GIT_COMMAND);
	git_tag_message->AddString("command", "tag");
	fGitTagItem->SetMessage(git_tag_message);

	fGitMenu->AddSeparatorItem();

	fGitMenu->AddItem(fGitLogOnelineItem = new BMenuItem(B_TRANSLATE("Log (Oneline)"), nullptr));
	BMessage* git_log_oneline_message = new BMessage(MSG_GIT_COMMAND);
	git_log_oneline_message->AddString("command", "log --oneline --decorate");
	fGitLogOnelineItem->SetMessage(git_log_oneline_message);

	fGitMenu->AddItem(fGitPullRebaseItem = new BMenuItem(B_TRANSLATE("Pull (Rebase)"), nullptr));
	BMessage* git_pull_rebase_message = new BMessage(MSG_GIT_COMMAND);
	git_pull_rebase_message->AddString("command", "pull --rebase");
	fGitPullRebaseItem->SetMessage(git_pull_rebase_message);

	fGitMenu->AddItem(fGitStatusShortItem = new BMenuItem(B_TRANSLATE("Status (Short)"), nullptr));
	BMessage* git_status_short_message = new BMessage(MSG_GIT_COMMAND);
	git_status_short_message->AddString("command", "status --short");
	fGitStatusShortItem->SetMessage(git_status_short_message);

	fGitMenu->SetEnabled(false);
	fMenuBar->AddItem(fGitMenu);
/*
	fHgMenu = new BMenu(B_TRANSLATE("Hg"));
	fHgMenu->AddItem(fHgStatusItem = new BMenuItem(B_TRANSLATE("Status"), nullptr));
	BMessage* hg_status_message = new BMessage(MSG_HG_COMMAND);
	hg_status_message->AddString("command", "status");
	fHgStatusItem->SetMessage(hg_status_message);

	fHgMenu->SetEnabled(false);
	menu->AddItem(fHgMenu);
*/

	BMenu* windowMenu = new BMenu(B_TRANSLATE("Window"));

	BMenu* submenu = new BMenu(B_TRANSLATE("Appearance"));
	submenu->AddItem(new BMenuItem(B_TRANSLATE("Toggle Projects panes"),
		new BMessage(MSG_SHOW_HIDE_PROJECTS)));
	submenu->AddItem(new BMenuItem(B_TRANSLATE("Toggle Output panes"),
		new BMessage(MSG_SHOW_HIDE_OUTPUT)));
	submenu->AddItem(new BMenuItem(B_TRANSLATE("Toggle ToolBar"),
		new BMessage(MSG_TOGGLE_TOOLBAR)));
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

	fToolBar->AddAction(MSG_SHOW_HIDE_PROJECTS, B_TRANSLATE("Show/Hide Projects split"), "kIconWindow");
	fToolBar->AddAction(MSG_SHOW_HIDE_OUTPUT,   B_TRANSLATE("Show/Hide Output split"),   "kIconTerminal");
	fToolBar->AddSeparator();
	fToolBar->AddAction(MSG_FILE_FOLD_TOGGLE,   B_TRANSLATE("Fold/unfold all"), "App_OpenTargetFolder");
	fToolBar->AddAction(B_UNDO, B_TRANSLATE("Undo"), "kIconUndo");
	fToolBar->AddAction(B_REDO, B_TRANSLATE("Redo"), "kIconRedo");
	fToolBar->AddAction(MSG_FILE_SAVE, B_TRANSLATE("Save current File"), "kIconSave");
	fToolBar->AddAction(MSG_FILE_SAVE_ALL, B_TRANSLATE("Save all Files"), "kIconSaveAll");
	fToolBar->AddSeparator();
	fToolBar->AddAction(MSG_BUILD_PROJECT, B_TRANSLATE("Build Project"), "kIconBuild");
	fToolBar->AddAction(MSG_RUN_TARGET, B_TRANSLATE("Run Project"), "kIconRun");
	fToolBar->AddAction(MSG_DEBUG_PROJECT, B_TRANSLATE("Debug Project"),	"kIconDebug");
	fToolBar->AddSeparator();
	fToolBar->AddAction(MSG_FIND_GROUP_TOGGLED, B_TRANSLATE("Find toggle (closes Replace bar if open)"), "kIconFind");
	fToolBar->AddAction(MSG_REPLACE_GROUP_TOGGLED, B_TRANSLATE("Replace toggle (leaves Find bar open)"), "kIconReplace");
	fToolBar->AddSeparator();
	fToolBar->AddAction(MSG_RUN_CONSOLE_PROGRAM_SHOW, B_TRANSLATE("Run console program"), "kConsoleApp");
	fToolBar->AddGlue();
	fToolBar->AddAction(MSG_BUILD_MODE, B_TRANSLATE("Build mode: Debug"), "kAppDebugger");
	fToolBar->AddAction(MSG_BUFFER_LOCK, B_TRANSLATE("Set buffer read-only"), "kIconUnlocked");
	fToolBar->AddSeparator();
	fToolBar->AddAction(MSG_FILE_PREVIOUS_SELECTED, B_TRANSLATE("Select previous File"), "kIconBack_1");
	fToolBar->AddAction(MSG_FILE_NEXT_SELECTED, B_TRANSLATE("Select next File"), "kIconForward_2");
	fToolBar->AddAction(MSG_FILE_CLOSE, B_TRANSLATE("Close File"), "kIconClose");
	fToolBar->AddAction(MSG_FILE_MENU_SHOW, B_TRANSLATE("Indexed File list"), "kIconFileList");
}

void
GenioWindow::_InitOutputSplit()
{
	// Output
	fOutputTabView = new BTabView("OutputTabview");
	
	fProblemsPanel = new ProblemsPanel();

	fBuildLogView = new ConsoleIOView(B_TRANSLATE("Build Log"), BMessenger(this));

	fConsoleIOView = new ConsoleIOView(B_TRANSLATE("Console I/O"), BMessenger(this));

	fOutputTabView->AddTab(fProblemsPanel);
	fOutputTabView->AddTab(fBuildLogView);
	fOutputTabView->AddTab(fConsoleIOView);
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

// As of release 0.7.5 3 Genio's Makefiles are managed:
// ## Genio haiku Makefile	 (Comment/Decomment DEBUGGER var)
// ## Genio simple Makefile  (Insert/Erase -g in CFLAGS var)
// ## Genio generic Makefile (Comment/Decomment Debug var)
//
// Just put these lines at Makefile (or makefile) top
// to allow that behaviour
void
GenioWindow::_MakefileSetBuildMode(bool isReleaseMode)
{
	// Should not happen
	if (fActiveProject == nullptr)
		return;

	BDirectory dir(fActiveProject->Path());
	BEntry entry;
	BPath path;
	entry_ref ref;
	char name[B_FILE_NAME_LENGTH];
	std::vector<std::string> file_lines;
	bool found = false;

	while ((dir.GetNextEntry(&entry)) == B_OK && found == false) {

		if (entry.IsDirectory())
			continue;

		entry.GetName(name);
		BString fileName(name);

		if (fileName == "makefile" || fileName == "Makefile") {

			entry.GetPath(&path);
			std::ifstream file_in(path.Path());
			std::string line;
			std::getline(file_in, line);
			file_lines.push_back(line + "\n");

			// Check first line to see if it is Genio's makefile
			if (line.find("## Genio haiku Makefile") == 0) {

				while (std::getline(file_in, line)) {
					// Empty line, just put back newline
					if (line.length() == 0) {
						file_lines.push_back("\n");
						continue;
					}
					// Calculate offset of first non-space
					std::size_t offset = line.find_first_not_of("\t ");
					std::string match = "DEBUGGER";
					if (isReleaseMode == false)
						match.insert(0, 1, '#');

					// Match found, push back accorded line
					if (line.substr(offset).find(match) == 0) {

						if (isReleaseMode == false)
							line.erase(0, 1);
						else
							line.insert(0, 1, '#');

						file_lines.push_back(line + "\n");

						// makefile-engine just recognizes DEBUGGER presence
						// and not value. When it will one may adapt this instead
						// std::ostringstream oss;
						// oss << "DEBUGGER := " << std::boolalpha << !isReleaseMode << "\n";
						// file_lines.push_back(oss.str());

						found = true;
					} else
						file_lines.push_back(line + "\n");
				}
				file_in.close();

			} else if (line.find("## Genio simple Makefile") == 0) {

				while (std::getline(file_in, line)) {
					// Empty line, just put back newline
					if (line.length() == 0) {
						file_lines.push_back("\n");
						continue;
					}
					// Calculate offset of first non-space
					std::size_t offset = line.find_first_not_of("\t ");
					// Match found
					if (line.substr(offset).find("CFLAGS") == 0) {
						// Ignore CFLAGS variable without '='
						if (Genio::_in_container('=', line) == false)
							continue;
						// Whatever after '=' sign is the starting point for parsing
						std::size_t start = line.find_first_of('=') + 1;
						// Save line start
						std::string line_start = line.substr(0, start);
						line_start.append(" ");

						// Tokenize args
						bool isToken = false;
						bool isComment = false;
						std::vector<std::string> tokens;
						std::string token_str;
						std::string comment_str;

						for (auto it = line.cbegin() + start; it != line.cend(); it++) {
							switch (*it) {
								// Start comment unless in comment already
								// Close possible open token
								case '#':
									if (isComment == true)
										comment_str.append(1, *it);
									else {
										isComment = true;
										comment_str = "#";
										if (isToken == true) {
											tokens.push_back(token_str);
											isToken = false;
										}
									}
									break;
								// Start a token unless in comment
								case '-':
									if (isComment == true)
										comment_str.append(1, *it);
									else {
										isToken = true;
										token_str = "-";
									}
									break;
								// Space found, close token if open unless in comment
								case ' ':
								case '\t':
									if (isComment == true)
										comment_str.append(1, *it);
									else if (isToken == true) {
										tokens.push_back(token_str);
										isToken = false;
									}
									break;
								// Continuation character found, preserve unless in comment
								// Close possible open token
								case '\\':
									if (isComment == true)
										comment_str.append(1, *it);
									else {
										if (isToken == true) {
											tokens.push_back(token_str);
											isToken = false;
										}
										tokens.push_back("\\");
									}
									break;
								default:
									if (isComment == true)
										comment_str.append(1, *it);
									else if (isToken == true)
										token_str.append(1, *it);
									break;
							}
						}
						// Close possible open token
						if (isToken == true)
							tokens.push_back(token_str);

						if (isReleaseMode == true) {
							line = line_start;
							std::ostringstream oss;
							for (auto it = tokens.cbegin(); it < tokens.cend(); it++) {
								// Exclude debug flag(s)
								if (*it == "-g")
									continue;
								// Don't put spaces after continuation char
								else if (*it == "\\")
									oss << *it;
								else
									oss << *it << ' ';
							}
							if (!comment_str.empty())
								oss << comment_str;

							line += oss.str();
						} else {
							// Debug mode
							line = line_start;
							std::ostringstream oss;
							// Put it at the beginning as last token could be
							// a continuation character.
							oss << "-g ";
							for (auto it = tokens.cbegin(); it < tokens.cend(); it++) {
								// Don't put spaces after continuation char
								if (*it == "\\")
									oss << *it;
								else
									oss << *it << ' ';
							}
							if (!comment_str.empty())
								oss << comment_str;

							line += oss.str();
						}
						found = true;
					}
					file_lines.push_back(line + "\n");
				}
				file_in.close();

			} else if (line.find("## Genio generic Makefile") == 0) {

				while (std::getline(file_in, line)) {
					// Empty line, just put back newline
					if (line.length() == 0) {
						file_lines.push_back("\n");
						continue;
					}
					// Calculate offset of first non-space
					std::size_t offset = line.find_first_not_of("\t ");
					std::string match = "Debug";
					if (isReleaseMode == false)
						match.insert(0, 1, '#');

					// Match found, push back accorded line
					if (line.substr(offset).find(match) == 0) {
						if (isReleaseMode == false)
							line.erase(0, 1);
						else
							line.insert(0, 1, '#');

						file_lines.push_back(line + "\n");
						found = true;
					} else
						file_lines.push_back(line + "\n");
				}
				file_in.close();

			} else {
				;// TODO: Not Genio's makefile
			}

		} else if (fileName.IStartsWith("MAKEFILE")) {
			;// TODO: Not Genio's makefile
		} else
			// Makefile not found
			continue;
	}

	// Rewrite makefile if needed
	if (found == true) {
		std::ofstream file_out(path.Path());
		if (!file_out.is_open())
			return;
		for (auto it = file_lines.cbegin(); it != file_lines.cend(); it++)
				file_out<< *it;
	}
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

	BString text;
	text
		 << B_TRANSLATE("Deleting item:")
		 << " \"" << name << "\"" << ".\n\n"
		 << B_TRANSLATE("After deletion the item will be lost.") << "\n"
		 << B_TRANSLATE("Do you really want to delete it?") << "\n";

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
				_FileClose(openedIndex, true);

		// Remove the entry
		if (entry.Exists()) {
			status_t status = entry.Remove();
			if (status != B_OK) {
				OKAlert("Delete item", BString("Could not delete ") << name, B_WARNING_ALERT);
				LogError("Could not delete %s (status = %d)", name, status);
			}
		}
	}
}


void
GenioWindow::_ProjectRenameFile()
{
	ProjectItem *item = fProjectsFolderBrowser->GetCurrentProjectItem();
	BRect rect = item->GetTextRect();
	

	TextControlFloater *tf = new TextControlFloater(fProjectsFolderBrowser->ConvertToScreen(rect), B_ALIGN_LEFT,
									be_plain_font, item->Text(), fProjectsFolderBrowser, 
									new BMessage(MSG_PROJECT_MENU_DO_RENAME_FILE),new BMessage('exit'));
	tf->SetTitle("");
	
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

	BString closed(B_TRANSLATE("Project close:"));
	BString name = project->Name();

	// Active project closed
	if (project == fActiveProject) {
		fActiveProject = nullptr;
		closed = B_TRANSLATE("Active project close:");
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
			_FileClose(index);
		}
	}	
	
	fProjectsFolderBrowser->ProjectFolderDepopulate(project);
	fProjectFolderObjectList->RemoveItem(project);
	
	project->Close();
	
	delete project;
	
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
		notification
			<< B_TRANSLATE("Project open fail:")
			<< "  "  << newProject->Name();
		_SendNotification( notification.String(), "PROJ_OPEN_FAIL");
		delete newProject;

		return;
	}

	fProjectsFolderBrowser->ProjectFolderPopulate(newProject);
	fProjectFolderObjectList->AddItem(newProject);

	BString opened(B_TRANSLATE("Project open:"));
	if (fProjectFolderObjectList->CountItems() == 1 || activate == true) {
		_ProjectFolderActivate(newProject);
		opened = B_TRANSLATE("Active project open:");
	}

	BString notification;
	notification << opened << "  " << newProject->Name() << " " << B_TRANSLATE("at") << " " << newProject->Path();
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
	if (returnStatus != B_OK)
		notification << B_TRANSLATE("An error occurred while opening Terminal and setting working directory to:") << itemPath;
	else
		notification << B_TRANSLATE("Terminal succesfully opened with working directory:") << itemPath;
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
			notification << "An error occurred while showing item in Tracker:" << directoryPath.Path();
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
GenioWindow::_ReplaceGroupShow()
{
	bool findGroupOpen = !fFindGroup->IsHidden();

	if (findGroupOpen == false)
		_FindGroupShow();
	
	if (fReplaceGroup->IsHidden()) {
		fReplaceGroup->Show();
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

void
GenioWindow::_ReplaceGroupToggled()
{
	bool replaceHidden = fReplaceGroup->IsHidden();
	if (replaceHidden)
		_ReplaceGroupShow();
	else
		fReplaceGroup->Hide();

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
	fBuildItem->SetEnabled(active);
	fCleanItem->SetEnabled(active);
	fBuildModeItem->SetEnabled(active);
	fMakeCatkeysItem->SetEnabled(active);
	fMakeBindcatalogsItem->SetEnabled(active);
	fFileNewMenuItem->SetEnabled(active);
	fToolBar->SetActionEnabled(MSG_BUILD_PROJECT, active);
	
	if (active == true) {

		// Is this a git project?
		if (fActiveProject->Git())
			fGitMenu->SetEnabled(true);
		else
			fGitMenu->SetEnabled(false);
			
		// Build mode
		bool releaseMode = (fActiveProject->GetBuildMode() == BuildMode::ReleaseMode);
		// Build mode menu
		fToolBar->SetActionEnabled(MSG_BUILD_MODE, !releaseMode);
		fDebugModeItem->SetMarked(!releaseMode);
		fReleaseModeItem->SetMarked(releaseMode);

		// Target exists: enable run button
		chdir(fActiveProject->Path());
		BEntry entry(fActiveProject->GetTarget());
		if (entry.Exists()) {
			fRunItem->SetEnabled(true);
			fToolBar->SetActionEnabled(MSG_RUN_TARGET, true);
			// Enable debug button in debug mode only
			fDebugItem->SetEnabled(!releaseMode);
			fToolBar->SetActionEnabled(MSG_DEBUG_PROJECT, !releaseMode);

		} else {
			fRunItem->SetEnabled(false);
			fDebugItem->SetEnabled(false);
			fToolBar->SetActionEnabled(MSG_RUN_TARGET, false);
			fToolBar->SetActionEnabled(MSG_DEBUG_PROJECT, false);
		}
	} else { // here project is inactive
		fRunItem->SetEnabled(false);		
		fDebugItem->SetEnabled(false);
		fGitMenu->SetEnabled(false);
		fFileNewMenuItem->SetEnabled(false);
		fToolBar->SetActionEnabled(MSG_RUN_TARGET, false);
		fToolBar->SetActionEnabled(MSG_DEBUG_PROJECT, false);
		fToolBar->SetActionEnabled(MSG_BUILD_MODE, false);
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
	fSaveMenuItem->SetEnabled(editor->IsModified());
	fUndoMenuItem->SetEnabled(editor->CanUndo());
	fRedoMenuItem->SetEnabled(editor->CanRedo());
	fCutMenuItem->SetEnabled(editor->CanCut());
	fCopyMenuItem->SetEnabled(editor->CanCopy());
	fPasteMenuItem->SetEnabled(editor->CanPaste());

	// ToolBar Items
	fToolBar->SetActionEnabled(B_UNDO, editor->CanUndo());
	fToolBar->SetActionEnabled(B_REDO, editor->CanRedo());
	fToolBar->SetActionEnabled(MSG_FILE_SAVE, editor->IsModified());

	// editor is modified by _FilesNeedSave so it should be the last
	// or reload editor pointer
	bool filesNeedSave = _FilesNeedSave();
	fToolBar->SetActionEnabled(MSG_FILE_SAVE_ALL, filesNeedSave);
	fSaveAllMenuItem->SetEnabled(filesNeedSave);
}

// Updating menu, toolbar, title.
// Also cleaning Status bar if no open files
void
GenioWindow::_UpdateTabChange(Editor* editor, const BString& caller)
{
	// All files are closed
	if (editor == nullptr) {
		// ToolBar Items
		fToolBar->SetActionEnabled(MSG_FIND_GROUP_TOGGLED, false);
		fToolBar->SetActionEnabled(MSG_REPLACE_GROUP_TOGGLED, false);
		fReplaceGroup->Hide();
		fToolBar->SetActionEnabled(MSG_FILE_FOLD_TOGGLE, false);
		fToolBar->SetActionEnabled(B_UNDO, false);
		fToolBar->SetActionEnabled(B_REDO, false);
		
		fToolBar->SetActionEnabled(MSG_FILE_SAVE, false);
		
		fToolBar->SetActionEnabled(MSG_FILE_SAVE_ALL, false);
		fToolBar->SetActionEnabled(MSG_BUFFER_LOCK, false);
		fToolBar->SetActionEnabled(MSG_FIND_PREVIOUS, false);
		fToolBar->SetActionEnabled(MSG_FIND_NEXT, false);
		fToolBar->SetActionEnabled(MSG_FILE_CLOSE, false);
		fToolBar->SetActionEnabled(MSG_FILE_MENU_SHOW, false);

		// Menu Items
		fSaveMenuItem->SetEnabled(false);
		fSaveAsMenuItem->SetEnabled(false);
		fSaveAllMenuItem->SetEnabled(false);
		fCloseMenuItem->SetEnabled(false);
		fFoldMenuItem->SetEnabled(false);
		fCloseAllMenuItem->SetEnabled(false);
		fUndoMenuItem->SetEnabled(false);
		fRedoMenuItem->SetEnabled(false);
		fCutMenuItem->SetEnabled(false);
		fCopyMenuItem->SetEnabled(false);
		fPasteMenuItem->SetEnabled(false);
		fSelectAllMenuItem->SetEnabled(false);
		fOverwiteItem->SetEnabled(false);
		fToggleWhiteSpacesItem->SetEnabled(false);
		fToggleLineEndingsItem->SetEnabled(false);
		fLineEndingsMenu->SetEnabled(false);
		fDuplicateLineItem->SetEnabled(false);
		fCommentSelectionItem->SetEnabled(false);
		fDeleteLinesItem->SetEnabled(false);
		fFindItem->SetEnabled(false);
		fReplaceItem->SetEnabled(false);
		fGoToLineItem->SetEnabled(false);
		fBookmarksMenu->SetEnabled(false);

		if (GenioNames::Settings.fullpath_title == true)
			SetTitle(GenioNames::kApplicationName);

		fProblemsPanel->Clear();
		fOutputTabView->TabAt(0)->SetLabel(fProblemsPanel->TabLabel());
		
		return;
	}

	// ToolBar Items
	
	fToolBar->SetActionEnabled(MSG_FIND_GROUP_TOGGLED, true);
	fToolBar->SetActionEnabled(MSG_REPLACE_GROUP_TOGGLED, true);
	fToolBar->SetActionEnabled(MSG_FILE_FOLD_TOGGLE, editor->IsFoldingAvailable());
	fToolBar->SetActionEnabled(B_UNDO, editor->CanUndo());
	fToolBar->SetActionEnabled(B_REDO, editor->CanRedo());
	fToolBar->SetActionEnabled(MSG_FILE_SAVE, editor->IsModified());
	fToolBar->SetActionEnabled(MSG_BUFFER_LOCK, !editor->IsReadOnly());
	fToolBar->SetActionEnabled(MSG_FILE_CLOSE, true);
	fToolBar->SetActionEnabled(MSG_FILE_MENU_SHOW, true);

	// Arrows
	/* TODO: remove! */
	int32 maxTabIndex = (fTabManager->CountTabs() - 1);
	int32 index = fTabManager->SelectedTabIndex();
	if (index == 0) {
		fToolBar->SetActionEnabled(MSG_FIND_PREVIOUS, false);
		if (maxTabIndex > 0)
				fToolBar->SetActionEnabled(MSG_FIND_NEXT, true);
	} else if (index == maxTabIndex) {
			fToolBar->SetActionEnabled(MSG_FIND_NEXT, false);
			fToolBar->SetActionEnabled(MSG_FIND_PREVIOUS, true);
	} else {
			fToolBar->SetActionEnabled(MSG_FIND_PREVIOUS, true);
			fToolBar->SetActionEnabled(MSG_FIND_NEXT, true);
	}
	/* END REMOVE */
	
	// Menu Items
	fSaveMenuItem->SetEnabled(editor->IsModified());
	fSaveAsMenuItem->SetEnabled(true);
	fCloseMenuItem->SetEnabled(true);
	fCloseAllMenuItem->SetEnabled(true);

	// Edit menu items
	fFoldMenuItem->SetEnabled(editor->IsFoldingAvailable());
	fUndoMenuItem->SetEnabled(editor->CanUndo());
	fRedoMenuItem->SetEnabled(editor->CanRedo());
	fCutMenuItem->SetEnabled(editor->CanCut());
	fCopyMenuItem->SetEnabled(editor->CanCopy());
	fPasteMenuItem->SetEnabled(editor->CanPaste());
	fSelectAllMenuItem->SetEnabled(true);
	fOverwiteItem->SetEnabled(true);
	// fOverwiteItem->SetMarked(editor->IsOverwrite());
	fToggleWhiteSpacesItem->SetEnabled(true);
	fToggleLineEndingsItem->SetEnabled(true);
	fLineEndingsMenu->SetEnabled(!editor->IsReadOnly());
	fDuplicateLineItem->SetEnabled(!editor->IsReadOnly());
	fCommentSelectionItem->SetEnabled(!editor->IsReadOnly());
	fDeleteLinesItem->SetEnabled(!editor->IsReadOnly());
	fFindItem->SetEnabled(true);
	fReplaceItem->SetEnabled(true);
	fGoToLineItem->SetEnabled(true);
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
	fToolBar->SetActionEnabled(MSG_FILE_SAVE_ALL, filesNeedSave);
	fSaveAllMenuItem->SetEnabled(filesNeedSave);

	BMessage diagnostics;
	editor->GetProblems(&diagnostics);
	fProblemsPanel->UpdateProblems(&diagnostics);
	fOutputTabView->TabAt(0)->SetLabel(fProblemsPanel->TabLabel());
	
	LogTraceF("called by: %s:%d", caller.String(), index);
}


status_t
GenioWindow::_CreateNewFile(BMessage *message, BEntry *dest)
{
	status_t status;

	BString type;
	status = message->FindString("type", &type);
	if (status != B_OK) {
		LogError("Can't find type!");
		return status;
	}
	
	if (type == "new_folder") {
		BDirectory dir(dest);
		status = dir.CreateDirectory("New folder", nullptr);
		if (status != B_OK) {
			OKAlert(B_TRANSLATE("New folder"), B_TRANSLATE("Error creating folder"), B_WARNING_ALERT);
			LogError("Invalid destination directory [%s]", dest->Name());
		}
		return status;
	}
	
	if (type == "new_file") {
		entry_ref new_ref;
		if (message->FindRef("refs", &new_ref) != B_OK) {
			LogError("Can't find ref in message!");
			return B_ERROR;
		}
		// Copy template file to destination
		BEntry sourceEntry(&new_ref);
		BPath destPath;
		dest->GetPath(&destPath);
		destPath.Append(new_ref.name, true);
		BEntry destEntry(destPath.Path());
		status = CopyFile(&sourceEntry, &destEntry, false);
		if (status != B_OK) {
			OKAlert(B_TRANSLATE("New folder"), B_TRANSLATE("Error creating new file"), B_WARNING_ALERT);
			LogError("Error creating new file %s in %s", new_ref.name, dest->Name());
		}
		return status;
	}
	
	if (type == "open_template_folder") {
		entry_ref new_ref;
		status = message->FindRef("refs", &new_ref);
		if (status != B_OK) {
			LogError("Can't find ref in message!");
			return status;
		}
		_ShowInTracker(&new_ref);
		return B_OK;
	}
	
	return B_OK;
}


void
GenioWindow::UpdateMenu()
{
	ProjectItem *item = fProjectsFolderBrowser->GetCurrentProjectItem();
	if (item != nullptr) {
		if (item->GetSourceItem()->Type() != SourceItemType::FileItem)
			fFileNewMenuItem->SetEnabled(true);
		else
			fFileNewMenuItem->SetEnabled(false);
	}
}
