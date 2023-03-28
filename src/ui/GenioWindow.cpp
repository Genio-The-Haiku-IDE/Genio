/*
 * Copyright 2022-2023 Nexus6 
 * Copyright 2017..2018 A. Mosca 
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

#include "exceptions/Exceptions.h"
#include "GenioCommon.h"
#include "GenioNamespace.h"
#include "GenioWindowMessages.h"
#include "Log.h"
#include "NewProjectWindow.h"
#include "ProjectSettingsWindow.h"
#include "ProjectFolder.h"
#include "ProjectItem.h"
#include "SettingsWindow.h"
#include "TPreferences.h"
#include "TextUtils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioWindow"


constexpr auto kRecentFilesNumber = 14 + 1;

// If enabled check menu open point
//static const auto kToolBarSize = 29;

static constexpr float kTabBarHeight = 30.0f;

static constexpr auto kGotolineMaxBytes = 6;

// Find group
static constexpr auto kFindReplaceMaxBytes = 50;
static constexpr auto kFindReplaceMinBytes = 32;
static constexpr float kFindReplaceOPSize = 120.0f;
static constexpr auto kFindReplaceMenuItems = 10;

static float kProjectsWeight  = 1.0f;
static float kEditorWeight  = 3.14f;
static float kOutputWeight  = 0.4f;

BRect dirtyFrameHack;


class ProjectRefFilter : public BRefFilter {

public:
//	virtual ~ProjectRefFilter()
//	{
//	}

	virtual bool Filter(const entry_ref* ref, BNode* node,
		struct stat_beos* stat, const char* filetype)
	{
		BString name(ref->name);
		return name.EndsWith(GenioNames::kProjectExtension); // ".idmpro"
	}
};

GenioWindow::GenioWindow(BRect frame)
	:
	BWindow(frame, "Genio", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS |
												B_QUIT_ON_WINDOW_CLOSE)
	, fActiveProject(nullptr)
	, fIsBuilding(false)
	, fConsoleStdinLine("")
	, fConsoleIOThread(nullptr)
	, fBuildLogView(nullptr)
	, fConsoleIOView(nullptr)
{
	// Settings file check.
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append(GenioNames::kApplicationName);
	path.Append(GenioNames::kSettingsFileName);
	bool settingsFileMissing = !Genio::file_exists(path.Path());

	// Fill Settings vars before using
	GenioNames::LoadSettingsVars();

	_InitMenu();

	_InitWindow();

	// Shortcuts
	for (int32 index = 1; index < 10; index++) {
		constexpr auto kAsciiPos {48};
		BMessage* selectTab = new BMessage(MSG_SELECT_TAB);
		selectTab->AddInt32("index", index - 1);
		AddShortcut(index + kAsciiPos, B_COMMAND_KEY, selectTab);
	}
	AddShortcut(B_LEFT_ARROW, B_OPTION_KEY, new BMessage(MSG_FILE_PREVIOUS_SELECTED));
	AddShortcut(B_RIGHT_ARROW, B_OPTION_KEY, new BMessage(MSG_FILE_NEXT_SELECTED));

	// Interface elements. If settings file is missing most probably it is
	// first time execution, load all elements
	if (settingsFileMissing == true) {
		fProjectsTabView->Show();
		fToolBar->View()->Show();
		fOutputTabView->Show();
	} else {
		if (GenioNames::Settings.show_projects == false)
			fProjectsTabView->Hide();
		if (GenioNames::Settings.show_output == false)
			fOutputTabView->Hide();
		if (GenioNames::Settings.show_toolbar == false)
			fToolBar->View()->Hide();
	}

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
	if (handler == fFindTextControl->TextView()) {
		if (message->what == B_KEY_DOWN) {
			int8 key;
			if (message->FindInt8("byte", 0, &key) == B_OK) {
				if (key == B_ESCAPE) {
					// If keep focus activated TODO
					#if 0
					int32 index = fTabManager->SelectedTabIndex();
					if (index > -1 && index < fTabManager->CountTabs()) {
						Editor* editor = fEditorObjectList->ItemAt(index);
						editor->GrabFocus();
					}
					#else
					{
						fFindGroup->SetVisible(false);
						fReplaceGroup->SetVisible(false);
					}
					#endif
				}
			}
		}
	} else if (handler == fReplaceTextControl->TextView()) {
		if (message->what == B_KEY_DOWN) {
			int8 key;
			if (message->FindInt8("byte", 0, &key) == B_OK) {
				if (key == B_ESCAPE) {
					fReplaceGroup->SetVisible(false);
					fFindTextControl->MakeFocus(true);
				}
			}
		}
	} else if (handler == fGotoLine->TextView()) {
		if (message->what == B_KEY_DOWN) {
			int8 key;
			if (message->FindInt8("byte", 0, &key) == B_OK) {
				if (key == B_ESCAPE) {
					fGotoLine->Hide();
					Editor* editor = fTabManager->SelectedEditor();
					if (editor) {
						editor->GrabFocus();
					}
				}
			}
		}
	} else if (handler == fConsoleIOView) {
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
	}

	BWindow::DispatchMessage(message, handler);
}

void
GenioWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
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
			_FileOpen(message, false);
			Activate();
			break;
		case B_SAVE_REQUESTED:
			_FileSaveAs(fTabManager->SelectedTabIndex(), message);
			break;
		/*case B_SELECT_ALL: {
			int32 index = fTabManager->SelectedTabIndex();

			if (index > -1 && index < fTabManager->CountTabs()) {
				editor = fEditorObjectList->ItemAt(index);
				editor->SelectAll();
			}
			break;
		}*/
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
		case CONSOLEIOTHREAD_STOP:
		{
			// TODO: Review focus policy
//			if (fTabManager->CountTabs() > 0)
//				editor->GrabFocus();

			fIsBuilding = false;
			fProjectsFolderBrowser->SetBuildingPhase(fIsBuilding);

			BString type;
			if (message->FindString("cmd_type", &type) == B_OK) {
				if (type == "build" || type == "clean" || type == "run") {
					_UpdateProjectActivation(true);
				} else if (type.StartsWith("git")) {
					_UpdateProjectActivation(true);
				} else if (type == "startfail") {
					if (fActiveProject != nullptr)
						_UpdateProjectActivation(true);
					break;
				} else if (type == "catkeys" || type == "bindcatalogs") {
					;
				} else {
					// user custom (run console program)
					;
				}
			}

			if (fConsoleIOThread) {
				fConsoleIOThread->InterruptExternal();
				fConsoleIOThread = nullptr;
			}
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
		case EDITOR_PRETEND_POSITION_CHANGED: {
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				int32 index =  _GetEditorIndex(&ref);
				if (index == fTabManager->SelectedTabIndex()) {
					int32 line, column;
					if (message->FindInt32("line", &line) == B_OK
						&& message->FindInt32("column", &column) == B_OK)

							_UpdateStatusBarText(line, column);
				}
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
					int32 line, column;
					// Enable Cut,Copy,Paste shortcuts
					_UpdateSavepointChange(index, "EDITOR_POSITION_CHANGED");
					if (message->FindInt32("line", &line) == B_OK
						&& message->FindInt32("column", &column) == B_OK)
							_UpdateStatusBarText(line, column);
				}
			}
			break;
		}
		case EDITOR_SAVEPOINT_LEFT: {
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				int32 index = _GetEditorIndex(&ref);
				if (index > -1) {
					_UpdateLabel(index, true);
					_UpdateSavepointChange(index, "Left");
				}
			}

			break;
		}
		case EDITOR_SAVEPOINT_REACHED: {
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				int32 index = _GetEditorIndex(&ref);
				if (index > -1) {
					_UpdateLabel(index, false);
					_UpdateSavepointChange(index, "Reached");
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
				editor->SetReadOnly();
				fFileUnlockedButton->SetEnabled(!editor->IsReadOnly());
				_UpdateStatusBarTrailing(fTabManager->SelectedTabIndex());
			}
			break;
		}
		case MSG_BUILD_MODE_DEBUG: {
			// fBuildModeButton->SetEnabled(true);
			fBuildModeButton->SetToolTip(B_TRANSLATE("Build mode: Debug"));
			fActiveProject->SetBuildMode(BuildMode::DebugMode);
			// _MakefileSetBuildMode(false);
			_UpdateProjectActivation(fActiveProject != nullptr);
			break;
		}
		case MSG_BUILD_MODE_RELEASE: {
			// fBuildModeButton->SetEnabled(false);
			fBuildModeButton->SetToolTip(B_TRANSLATE("Build mode: Release"));
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
				_UpdateStatusBarTrailing(fTabManager->SelectedTabIndex());
			}
			break;
		}
		case MSG_EOL_SET_TO_DOS: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->SetEndOfLine(SC_EOL_CRLF);
				_UpdateStatusBarTrailing(fTabManager->SelectedTabIndex());
			}
			break;
		}
		case MSG_EOL_SET_TO_MAC: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {
				editor->SetEndOfLine(SC_EOL_CR);
				_UpdateStatusBarTrailing(fTabManager->SelectedTabIndex());
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
				BRect buttonFrame = fFileMenuButton->Frame();
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
		case MSG_FILE_NEW: {
			//TODO
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
		case MSG_FIND_NEXT: {
			const BString& text(fFindTextControl->Text());
//			if (!text.IsEmpty())
			_FindNext(text, false);
			break;
		}
		case MSG_FIND_PREVIOUS: {
			const BString& text(fFindTextControl->Text());
//			if (!text.IsEmpty())
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
		case MSG_GOTO_LINE:
			fGotoLine->Show();
			fGotoLine->MakeFocus();
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
		case MSG_LINE_TO_GOTO: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) {

				std::string linestr(fGotoLine->Text());
				int32 line;
				std::istringstream (linestr) >>  line;

				if (line <= editor->CountLines())
					editor->GoToLine(line);

				editor->GrabFocus();
				fGotoLine->SetText("");
				fGotoLine->Hide();
			}
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
		case MSG_PROJECT_MENU_CLOSE: {
			_ProjectFolderClose(fProjectsFolderBrowser->GetProjectFromCurrentItem());
			break;
		}
		case MSG_PROJECT_MENU_DELETE_FILE: {
			_ProjectFileDelete();
			break;
		}
		case MSG_PROJECT_MENU_EXCLUDE_FILE: {
			_ProjectFileExclude();
			break;
		}
		case MSG_PROJECT_MENU_OPEN_FILE: {
			_ProjectFileOpen(_ProjectFileFullPath());
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
		case MSG_PROJECT_NEW: {
			// NewProjectWindow *wnd = new NewProjectWindow();
			// wnd->Show();
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
		case MSG_TEXT_DELETE: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor) 
				editor->Clear();

			break;
		}
		case MSG_TEXT_OVERWRITE: {
			Editor* editor = fTabManager->SelectedEditor();
			if (editor)  {
				editor->OverwriteToggle();
				_UpdateStatusBarTrailing(fTabManager->SelectedTabIndex());
			}
			break;
		}

		case MSG_TOGGLE_TOOLBAR: {
			if (fToolBar->View()->IsHidden()) {
				fToolBar->View()->Show();
			} else {
				fToolBar->View()->Hide();
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
		case GenioNames::ADDTOPROJECTWINDOW_NEW_ITEM: {
			BString projectName, filePath, file2Path;
			if (message->FindString("project", &projectName) == B_OK
				&& message->FindString("file_path", &filePath) == B_OK) {
					_ProjectFileOpen(filePath);
			}
			if ( message->FindString("file2_path", &file2Path) == B_OK)
				_ProjectFileOpen(file2Path);

			break;
		}
		case TABMANAGER_TAB_SELECTED: {
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				Editor* editor = fTabManager->EditorAt(index);
				// TODO notify and check index too
				if (editor == nullptr) {
					break;
				}	
				editor->GrabFocus();
				// In multifile open not-focused files place scroll just after
				// caret line when reselected. Ensure visibility.
				// TODO caret policy
				editor->EnsureVisiblePolicy();

				editor->PretendPositionChanged();
				_UpdateTabChange(index, "TABMANAGER_TAB_SELECTED");
				_UpdateStatusBarTrailing(index);
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
	

	GenioNames::Settings.find_wrap = (bool)fFindWrapCheck->Value();
	GenioNames::Settings.find_whole_word = (bool)fFindWholeWordCheck->Value();
	GenioNames::Settings.find_match_case = (bool)fFindCaseSensitiveCheck->Value();
	

	GenioNames::SaveSettingsVars();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


status_t
GenioWindow::_AddEditorTab(entry_ref* ref, int32 index, int32 be_line, int lsp_char)
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
	status_t status;

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

	fConsoleIOThread = new ConsoleIOThread(&message,  BMessenger(this),
		BMessenger(fBuildLogView));

	status = fConsoleIOThread->Start();

	return status;
}

status_t
GenioWindow::_CleanProject()
{
	status_t status;

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

	fConsoleIOThread = new ConsoleIOThread(&message,  BMessenger(this),
		BMessenger(fBuildLogView));

	status = fConsoleIOThread->Start();

	return status;
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
		_UpdateTabChange(-1, "_FileClose");

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
GenioWindow::_FileOpen(BMessage* msg, bool openWithPreferred)
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

	while (msg->FindRef("refs", refsCount++, &ref) == B_OK) {

		if (!_FileIsSupported(&ref)) {
			if (openWithPreferred)
				_FileOpenWithPreferredApp(&ref); //TODO make this optional?
			continue;
		}	

		refsCount++;

		// Do not reopen an already opened file
		if ((openedIndex = _GetEditorIndex(&ref)) != -1) {
			if (openedIndex != fTabManager->SelectedTabIndex()) {
				fTabManager->SelectTab(openedIndex);
			}				
			
			if (lsp_char >= 0 && be_line > -1)
				fTabManager->EditorAt(openedIndex)->GoToLSPPosition(be_line - 1, lsp_char);
			else
			if (be_line > -1)
				fTabManager->EditorAt(openedIndex)->GoToLine(be_line);
			
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
		mime[0]='\0';
		info.GetType(mime);
		if (mime[0] == '\0' && update_mime_info(BPath(ref).Path(), false, true, B_UPDATE_MIME_INFO_NO_FORCE) == B_OK)
		{
			info.GetType(mime);
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
	editor->StopMonitoring();

	ssize_t written = editor->SaveToFile();
	ssize_t length = editor->SendMessage(SCI_GETLENGTH, 0, 0);

	// Restart monitoring
	editor->StartMonitoring();

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
	if (!fFindGroup->IsVisible()) {
		fFindGroup->SetVisible(true);
		_GetFocusAndSelection(fFindTextControl);
	}
	else if (fFindGroup->IsVisible())
		_GetFocusAndSelection(fFindTextControl);
}

void
GenioWindow::_FindGroupToggled()
{
	fFindGroup->SetVisible(!fFindGroup->IsVisible());

	if (fFindGroup->IsVisible()) {
		_GetFocusAndSelection(fFindTextControl);
	}
	else {
		if (fReplaceGroup->IsVisible())
			fReplaceGroup->SetVisible(false);
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
//fFindTextControl->MakeFocus(true);
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

	  text.CharacterEscape("\'\\ \n\"", '\\');

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
	if (editor->IsTextSelected()) {
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
	status_t status;
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

	fConsoleIOThread = new ConsoleIOThread(&message,  BMessenger(this),
		BMessenger(fConsoleIOView));

	status = fConsoleIOThread->Start();

	return status;
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
std::cerr << __PRETTY_FUNCTION__ << " MODES" << std::endl;
		break;
	case B_STAT_SIZE:
std::cerr << __PRETTY_FUNCTION__ << " B_STAT_SIZE" << std::endl;
		break;
	case B_STAT_ACCESS_TIME:
std::cerr << __PRETTY_FUNCTION__ << " B_STAT_ACCESS_TIME" << std::endl;
		break;
	case B_STAT_MODIFICATION_TIME:
std::cerr << __PRETTY_FUNCTION__ << " B_STAT_MODIFICATION_TIME" << std::endl;
		break;
	case B_STAT_CREATION_TIME:
std::cerr << __PRETTY_FUNCTION__ << " B_STAT_CREATION_TIME" << std::endl;
		break;
	case B_STAT_CHANGE_TIME:
std::cerr << __PRETTY_FUNCTION__ << " B_STAT_CHANGE_TIME" << std::endl;
		break;
	case B_STAT_INTERIM_UPDATE:
std::cerr << __PRETTY_FUNCTION__ << " B_STAT_INTERIM_UPDATE" << std::endl;
		break;
	default:
std::cerr << __PRETTY_FUNCTION__ << "fields is: 0x" << std::hex << fields << std::endl;
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

	fFindNextButton = _LoadIconButton("FindNextButton", MSG_FIND_NEXT, 164, true,
						B_TRANSLATE("Find Next"));
	fFindPreviousButton = _LoadIconButton("FindPreviousButton", MSG_FIND_PREVIOUS,
							165, true, B_TRANSLATE("Find previous"));
	fFindMarkAllButton = _LoadIconButton("FindMarkAllButton", MSG_FIND_MARK_ALL,
							202, true, B_TRANSLATE("Mark all"));
	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY, new BMessage(MSG_FIND_NEXT));
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY, new BMessage(MSG_FIND_PREVIOUS));
	// AddShortcut(B_PAGE_DOWN, B_COMMAND_KEY, new BMessage(MSG_FIND_NEXT));
	// AddShortcut(B_PAGE_UP, B_COMMAND_KEY, new BMessage(MSG_FIND_PREVIOUS));

	fFindCaseSensitiveCheck = new BCheckBox(B_TRANSLATE("Match case"));
	fFindWholeWordCheck = new BCheckBox(B_TRANSLATE("Whole word"));
	fFindWrapCheck = new BCheckBox(B_TRANSLATE("Wrap"));
	
	fFindWrapCheck->SetValue((int32)GenioNames::Settings.find_wrap);
	fFindWholeWordCheck->SetValue((int32)GenioNames::Settings.find_whole_word);
	fFindCaseSensitiveCheck->SetValue((int32)GenioNames::Settings.find_match_case);
	
	fFindinFilesButton = _LoadIconButton("FindinFiles", MSG_FIND_IN_FILES, 201, true,
						B_TRANSLATE("Find in files"));
	
	fFindGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0f)
		.Add(BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
			.Add(fFindMenuField)
			.Add(fFindTextControl)
			.Add(fFindNextButton)
			.Add(fFindPreviousButton)
			.Add(fFindWrapCheck)
			.Add(fFindWholeWordCheck)
			.Add(fFindCaseSensitiveCheck)
			.Add(fFindMarkAllButton)
			.Add(fFindinFilesButton)
			.AddGlue()
		)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
	;
	fFindGroup->SetVisible(false);

	// Replace group
	fReplaceMenuField = new BMenuField("ReplaceMenu", NULL,
										new BMenu(B_TRANSLATE("Replace:")));
	fReplaceMenuField->SetExplicitMaxSize(BSize(kFindReplaceOPSize, B_SIZE_UNSET));
	fReplaceMenuField->SetExplicitMinSize(BSize(kFindReplaceOPSize, B_SIZE_UNSET));

	fReplaceTextControl = new BTextControl("ReplaceTextControl", "", "", NULL);
	fReplaceTextControl->TextView()->SetMaxBytes(kFindReplaceMaxBytes);
	fReplaceTextControl->SetExplicitMaxSize(fFindTextControl->MaxSize());
	fReplaceTextControl->SetExplicitMinSize(fFindTextControl->MinSize());

	fReplaceOneButton = _LoadIconButton("ReplaceOneButton", MSG_REPLACE_ONE,
						166, true, B_TRANSLATE("Replace selection"));
	fReplaceAndFindNextButton = _LoadIconButton("ReplaceFindNextButton", MSG_REPLACE_NEXT,
								167, true, B_TRANSLATE("Replace and find next"));
	fReplaceAndFindPrevButton = _LoadIconButton("ReplaceFindPrevButton", MSG_REPLACE_PREVIOUS,
								168, true, B_TRANSLATE("Replace and find previous"));
	fReplaceAllButton = _LoadIconButton("ReplaceAllButton", MSG_REPLACE_ALL,
							169, true, B_TRANSLATE("Replace all"));

	fReplaceGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0f)
		.Add(BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
			.Add(fReplaceMenuField)
			.Add(fReplaceTextControl)
			.Add(fReplaceOneButton)
			.Add(fReplaceAndFindNextButton)
			.Add(fReplaceAndFindPrevButton)
			.Add(fReplaceAllButton)
			.AddGlue()
		)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
	;
	fReplaceGroup->SetVisible(false);

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

	// Status Bar
	fStatusBar = new BStatusBar("StatusBar");
	fStatusBar->SetBarHeight(1.0);

	fEditorTabsGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0f)
		.Add(fRunConsoleProgramGroup)
		.Add(fFindGroup)
		.Add(fReplaceGroup)
		.Add(fTabManager->TabGroup())
		.Add(fTabManager->ContainerView())
		.Add(fStatusBar)
	;

}

void
GenioWindow::_InitMenu()
{
	// Menu
	fMenuBar = new BMenuBar("menubar");

	BMenu* menu = new BMenu(B_TRANSLATE("Project"));
	// TODO: As a temporary measure we disable New menu item until we merge
	// the project-folders branch into main and implement a brand new system
	// to create new projects. This will likely be based on the "template" or 
	// "stationery" concept as found in Paladin or BeIDE
	// menu->AddItem(new BMenuItem(B_TRANSLATE("New"),
		// new BMessage(MSG_PROJECT_NEW), 'N', B_OPTION_KEY));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(MSG_PROJECT_OPEN), 'O', B_OPTION_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings"),
		new BMessage(MSG_PROJECT_SETTINGS)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));

	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("File"));
	menu->AddItem(fFileNewMenuItem = new BMenuItem(B_TRANSLATE("New"),
		new BMessage(MSG_FILE_NEW)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(MSG_FILE_OPEN), 'O'));
	menu->AddItem(new BMenuItem(BRecentFilesList::NewFileListMenu(
			B_TRANSLATE("Open recent" B_UTF8_ELLIPSIS), nullptr, nullptr, this,
			kRecentFilesNumber, true, nullptr, GenioNames::kApplicationSignature), nullptr));
	menu->AddSeparatorItem();
	menu->AddItem(fSaveMenuItem = new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(MSG_FILE_SAVE), 'S'));
	menu->AddItem(fSaveAsMenuItem = new BMenuItem(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS),
		new BMessage(MSG_FILE_SAVE_AS)));
	menu->AddItem(fSaveAllMenuItem = new BMenuItem(B_TRANSLATE("Save all"),
		new BMessage(MSG_FILE_SAVE_ALL), 'S', B_SHIFT_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(fCloseMenuItem = new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(MSG_FILE_CLOSE), 'W'));
	menu->AddItem(fCloseAllMenuItem = new BMenuItem(B_TRANSLATE("Close all"),
		new BMessage(MSG_FILE_CLOSE_ALL), 'W', B_SHIFT_KEY));
	fFileNewMenuItem->SetEnabled(false);

	menu->AddSeparatorItem();
	menu->AddItem(fFoldMenuItem = new BMenuItem(B_TRANSLATE("Fold"),
		new BMessage(MSG_FILE_FOLD_TOGGLE)));

	fSaveMenuItem->SetEnabled(false);
	fSaveAsMenuItem->SetEnabled(false);
	fSaveAllMenuItem->SetEnabled(false);
	fCloseMenuItem->SetEnabled(false);
	fCloseAllMenuItem->SetEnabled(false);
	fFoldMenuItem->SetEnabled(false);

	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Edit"));
	menu->AddItem(fUndoMenuItem = new BMenuItem(B_TRANSLATE("Undo"),
		new BMessage(B_UNDO), 'Z'));
	menu->AddItem(fRedoMenuItem = new BMenuItem(B_TRANSLATE("Redo"),
		new BMessage(B_REDO), 'Z', B_SHIFT_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(fCutMenuItem = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	menu->AddItem(fCopyMenuItem = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
	menu->AddItem(fPasteMenuItem = new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE), 'V'));
	menu->AddItem(fDeleteMenuItem = new BMenuItem(B_TRANSLATE("Delete"),
		new BMessage(MSG_TEXT_DELETE), 'D'));
	menu->AddSeparatorItem();
	menu->AddItem(fSelectAllMenuItem = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));
	menu->AddSeparatorItem();
	menu->AddItem(fOverwiteItem = new BMenuItem(B_TRANSLATE("Overwrite"),
		new BMessage(MSG_TEXT_OVERWRITE), B_INSERT));

	menu->AddSeparatorItem();
	menu->AddItem(fToggleWhiteSpacesItem = new BMenuItem(B_TRANSLATE("Toggle white spaces"),
		new BMessage(MSG_WHITE_SPACES_TOGGLE)));
	menu->AddItem(fToggleLineEndingsItem = new BMenuItem(B_TRANSLATE("Toggle line endings"),
		new BMessage(MSG_LINE_ENDINGS_TOGGLE)));
	menu->AddItem(fDuplicateLineItem = new BMenuItem(B_TRANSLATE("Duplicate current line"),
		new BMessage(MSG_DUPLICATE_LINE), 'K', B_OPTION_KEY));
	menu->AddItem(fDeleteLinesItem = new BMenuItem(B_TRANSLATE("Delete lines"),
		new BMessage(MSG_DELETE_LINES), 'D', B_OPTION_KEY));	
	menu->AddItem(fCommentSelectionItem = new BMenuItem(B_TRANSLATE("Comment selected lines"),
		new BMessage(MSG_COMMENT_SELECTED_LINES), 'C', B_OPTION_KEY));
	
	menu->AddSeparatorItem();	

	menu->AddItem(new BMenuItem(B_TRANSLATE("Autocompletion"), new BMessage(MSG_AUTOCOMPLETION), B_SPACE));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Format"), new BMessage(MSG_FORMAT)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Go to definition"), new BMessage(MSG_GOTODEFINITION), 'G'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Go to declaration"), new BMessage(MSG_GOTODECLARATION)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Go to implementation"), new BMessage(MSG_GOTOIMPLEMENTATION)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Switch Source Header"), new BMessage(MSG_SWITCHSOURCE), B_TAB));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Signature Help"), new BMessage(MSG_SIGNATUREHELP), '?'));


	menu->AddSeparatorItem();
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
	fDeleteMenuItem->SetEnabled(false);
	fSelectAllMenuItem->SetEnabled(false);
	fOverwiteItem->SetEnabled(false);
	fToggleWhiteSpacesItem->SetEnabled(false);
	fToggleLineEndingsItem->SetEnabled(false);
	fLineEndingsMenu->SetEnabled(false);

	menu->AddItem(fLineEndingsMenu);
	fMenuBar->AddItem(menu);
	
	menu = new BMenu(B_TRANSLATE("View"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Zoom In"), new BMessage(MSG_VIEW_ZOOMIN), '+'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Zoom Out"), new BMessage(MSG_VIEW_ZOOMOUT), '-'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Zoom Reset"), new BMessage(MSG_VIEW_ZOOMRESET), '0'));
	fMenuBar->AddItem(menu);
	
	menu = new BMenu(B_TRANSLATE("Search"));

	menu->AddItem(fFindItem = new BMenuItem(B_TRANSLATE("Find"),
		new BMessage(MSG_FIND_GROUP_SHOW), 'F'));
	menu->AddItem(fReplaceItem = new BMenuItem(B_TRANSLATE("Replace"),
		new BMessage(MSG_REPLACE_GROUP_SHOW), 'R'));
	menu->AddItem(fGoToLineItem = new BMenuItem(B_TRANSLATE("Go to line" B_UTF8_ELLIPSIS),
		new BMessage(MSG_GOTO_LINE), '<'));

	fBookmarksMenu = new BMenu(B_TRANSLATE("Bookmark"));
	fBookmarksMenu->AddItem(fBookmarkToggleItem = new BMenuItem(B_TRANSLATE("Toggle"),
		new BMessage(MSG_BOOKMARK_TOGGLE)));
	fBookmarksMenu->AddItem(fBookmarkClearAllItem = new BMenuItem(B_TRANSLATE("Clear all"),
		new BMessage(MSG_BOOKMARK_CLEAR_ALL)));
	fBookmarksMenu->AddItem(fBookmarkGoToNextItem = new BMenuItem(B_TRANSLATE("Go to next"),
		new BMessage(MSG_BOOKMARK_GOTO_NEXT)));
	fBookmarksMenu->AddItem(fBookmarkGoToPreviousItem = new BMenuItem(B_TRANSLATE("Go to previous"),
		new BMessage(MSG_BOOKMARK_GOTO_PREVIOUS)));

	fFindItem->SetEnabled(false);
	fReplaceItem->SetEnabled(false);
	fGoToLineItem->SetEnabled(false);
	fBookmarksMenu->SetEnabled(false);

	menu->AddItem(fBookmarksMenu);
	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Build"));
	menu->AddItem(fBuildItem = new BMenuItem (B_TRANSLATE("Build Project"),
		new BMessage(MSG_BUILD_PROJECT), 'B'));
	menu->AddItem(fCleanItem = new BMenuItem (B_TRANSLATE("Clean Project"),
		new BMessage(MSG_CLEAN_PROJECT)));
	menu->AddItem(fRunItem = new BMenuItem (B_TRANSLATE("Run target"),
		new BMessage(MSG_RUN_TARGET)));
	menu->AddSeparatorItem();

	fBuildModeItem = new BMenu(B_TRANSLATE("Build mode"));
	fBuildModeItem->SetRadioMode(true);
	fBuildModeItem->AddItem(fReleaseModeItem = new BMenuItem(B_TRANSLATE("Release"),
		new BMessage(MSG_BUILD_MODE_RELEASE)));
	fBuildModeItem->AddItem(fDebugModeItem = new BMenuItem(B_TRANSLATE("Debug"),
		new BMessage(MSG_BUILD_MODE_DEBUG)));
	fDebugModeItem->SetMarked(true);
	menu->AddItem(fBuildModeItem);
	menu->AddSeparatorItem();

	menu->AddItem(fDebugItem = new BMenuItem (B_TRANSLATE("Debug Project"),
		new BMessage(MSG_DEBUG_PROJECT)));
	menu->AddSeparatorItem();
	menu->AddItem(fMakeCatkeysItem = new BMenuItem ("make catkeys",
		new BMessage(MSG_MAKE_CATKEYS)));
	menu->AddItem(fMakeBindcatalogsItem = new BMenuItem ("make bindcatalogs",
		new BMessage(MSG_MAKE_BINDCATALOGS)));

	fBuildItem->SetEnabled(false);
	fCleanItem->SetEnabled(false);
	fRunItem->SetEnabled(false);
	fBuildModeItem->SetEnabled(false);
	fDebugItem->SetEnabled(false);
	fMakeCatkeysItem->SetEnabled(false);
	fMakeBindcatalogsItem->SetEnabled(false);

	fMenuBar->AddItem(menu);

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

	menu = new BMenu(B_TRANSLATE("Window"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings"),
		new BMessage(MSG_WINDOW_SETTINGS), 'P', B_OPTION_KEY));
	BMenu* submenu = new BMenu(B_TRANSLATE("Interface"));
	submenu->AddItem(new BMenuItem(B_TRANSLATE("Toggle Projects panes"),
		new BMessage(MSG_SHOW_HIDE_PROJECTS)));
	submenu->AddItem(new BMenuItem(B_TRANSLATE("Toggle Output panes"),
		new BMessage(MSG_SHOW_HIDE_OUTPUT)));
	submenu->AddItem(new BMenuItem(B_TRANSLATE("Toggle ToolBar"),
		new BMessage(MSG_TOGGLE_TOOLBAR)));
	menu->AddItem(submenu);

	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Help"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED)));

	fMenuBar->AddItem(menu);
}

void
GenioWindow::_InitToolbar()
{
	// toolbar group
	fProjectsButton = _LoadIconButton("ProjectsButton", MSG_SHOW_HIDE_PROJECTS,
						111, true, B_TRANSLATE("Show/Hide Projects split"));
	fOutputButton = _LoadIconButton("OutputButton", MSG_SHOW_HIDE_OUTPUT,
						115, true, B_TRANSLATE("Show/Hide Output split"));
	fBuildButton = _LoadIconButton("Build", MSG_BUILD_PROJECT,
						112, false, B_TRANSLATE("Build Project"));
	fRunButton = _LoadIconButton("Run", MSG_RUN_TARGET,
						113, false, B_TRANSLATE("Run Project"));
	fDebugButton = _LoadIconButton("Debug", MSG_DEBUG_PROJECT,
						114, false, B_TRANSLATE("Debug Project"));

	fFindButton = _LoadIconButton("Find", MSG_FIND_GROUP_TOGGLED, 199, false,
						B_TRANSLATE("Find toggle (closes Replace bar if open)"));

	fReplaceButton = _LoadIconButton("Replace", MSG_REPLACE_GROUP_TOGGLED, 200, false,
						B_TRANSLATE("Replace toggle (leaves Find bar open)"));


	fFoldButton = _LoadIconButton("Fold", MSG_FILE_FOLD_TOGGLE, 213, false,
						B_TRANSLATE("Fold toggle"));
	fUndoButton = _LoadIconButton("UndoButton", B_UNDO, 204, false,
						B_TRANSLATE("Undo"));
	fRedoButton = _LoadIconButton("RedoButton", B_REDO, 205, false,
						B_TRANSLATE("Redo"));
	fFileSaveButton = _LoadIconButton("FileSaveButton", MSG_FILE_SAVE,
						206, false, B_TRANSLATE("Save current File"));
	fFileSaveAllButton = _LoadIconButton("FileSaveAllButton", MSG_FILE_SAVE_ALL,
						207, false, B_TRANSLATE("Save all Files"));

	fConsoleButton = _LoadIconButton("ConsoleButton", MSG_RUN_CONSOLE_PROGRAM_SHOW,
		227, true, B_TRANSLATE("Run console program"));

	fBuildModeButton = _LoadIconButton("BuildMode", MSG_BUILD_MODE,
						221, false, B_TRANSLATE("Build mode: Debug"));

	fFileUnlockedButton = _LoadIconButton("FileUnlockedButton", MSG_BUFFER_LOCK,
						212, false, B_TRANSLATE("Set buffer read-only"));
	fFilePreviousButton = _LoadIconButton("FilePreviousButton", MSG_FILE_PREVIOUS_SELECTED,
						208, false, B_TRANSLATE("Select previous File"));
	fFileNextButton = _LoadIconButton("FileNextButton", MSG_FILE_NEXT_SELECTED,
						209, false, B_TRANSLATE("Select next File"));
	fFileCloseButton = _LoadIconButton("FileCloseButton", MSG_FILE_CLOSE,
						210, false, B_TRANSLATE("Close File"));
	fFileMenuButton = _LoadIconButton("FileMenuButton", MSG_FILE_MENU_SHOW,
						211, false, B_TRANSLATE("Indexed File list"));

	fGotoLine = new BTextControl("GotoLine", nullptr, nullptr, new BMessage(MSG_LINE_TO_GOTO));
	fGotoLine->SetToolTip(B_TRANSLATE("Go to line" B_UTF8_ELLIPSIS));
	float stringWidth = fGotoLine->StringWidth(" 123456") + 24.0f;
	fGotoLine->SetExplicitMaxSize(BSize(stringWidth, 24.0f)); //B_SIZE_UNSET
	for (auto i = 0; i < 256; i++)
		if (i < '0' || i > '9')
			fGotoLine->TextView()->DisallowChar(i);
	fGotoLine->TextView()->SetMaxBytes(kGotolineMaxBytes);
	fGotoLine->TextView()->SetAlignment(B_ALIGN_RIGHT);
	fGotoLine->Hide();

	fToolBar = BLayoutBuilder::Group<>(B_VERTICAL, 0)
		.Add(BLayoutBuilder::Group<>(B_HORIZONTAL, 1)
			.AddGlue()
			.Add(fProjectsButton)
			.Add(fOutputButton)
			.Add(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER))
			.Add(fFoldButton)
			.Add(fUndoButton)
			.Add(fRedoButton)
			.Add(fFileSaveButton)
			.Add(fFileSaveAllButton)
			.Add(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER))
			.Add(fBuildButton)
			.Add(fRunButton)
			.Add(fDebugButton)
			.Add(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER))
			.Add(fFindButton)
			.Add(fReplaceButton)
			.Add(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER))
			.Add(fConsoleButton)
			.AddGlue()
			.Add(fBuildModeButton)
			.Add(fFileUnlockedButton)
			.Add(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER))
			.Add(fGotoLine)
			.Add(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER))
			.Add(fFilePreviousButton)
			.Add(fFileNextButton)
			.Add(fFileCloseButton)
			.Add(fFileMenuButton)
//			.SetInsets(1, 1, 1, 1)
		)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
	;
}

void
GenioWindow::_InitOutputSplit()
{
	// Output
	fOutputTabView = new BTabView("OutputTabview");

	fNotificationsListView = new BColumnListView(B_TRANSLATE("Notifications"),
									B_NAVIGABLE, B_FANCY_BORDER, true);
	fNotificationsListView->AddColumn(new BDateColumn(B_TRANSLATE("Time"),
								200.0, 200.0, 200.0), kTimeColumn);
	fNotificationsListView->AddColumn(new BStringColumn(B_TRANSLATE("Message"),
								600.0, 600.0, 800.0, 0), kMessageColumn);
	fNotificationsListView->AddColumn(new BStringColumn(B_TRANSLATE("Type"),
								200.0, 200.0, 200.0, 0), kTypeColumn);
	BFont font;
	font.SetFamilyAndStyle("Noto Sans Mono", "Bold");
	fNotificationsListView->SetFont(&font);

	fBuildLogView = new ConsoleIOView(B_TRANSLATE("Build Log"), BMessenger(this));

	fConsoleIOView = new ConsoleIOView(B_TRANSLATE("Console I/O"), BMessenger(this));

	fOutputTabView->AddTab(fNotificationsListView);
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

BIconButton*
GenioWindow::_LoadIconButton(const char* name, int32 msg,
								int32 resIndex, bool enabled, const char* tooltip)
{
	BIconButton* button = new BIconButton(name, nullptr, new BMessage(msg));
//	button->SetIcon(_LoadSizedVectorIcon(resIndex, kToolBarSize));
	button->SetIcon(resIndex);
	button->SetEnabled(enabled);
	button->SetToolTip(tooltip);

	return button;
}

BBitmap*
GenioWindow::_LoadSizedVectorIcon(int32 resourceID, int32 size)
{
	BResources* res = BApplication::AppResources();
	size_t iconSize;
	const void* data = res->LoadResource(B_VECTOR_ICON_TYPE, resourceID, &iconSize);

	assert(data != nullptr);

	BBitmap* bitmap = new BBitmap(BRect(0, 0, size, size), B_RGBA32);

	status_t status = BIconUtils::GetVectorIcon(static_cast<const uint8*>(data),
						iconSize, bitmap);

	assert(status == B_OK);

	return bitmap;
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

	fConsoleIOThread = new ConsoleIOThread(&message,  BMessenger(this),
		BMessenger(fBuildLogView));

	fConsoleIOThread->Start();
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

	fConsoleIOThread = new ConsoleIOThread(&message,  BMessenger(this),
		BMessenger(fBuildLogView));

	fConsoleIOThread->Start();
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


/*void
GenioWindow::_ProjectClose()
{
	Project* project = _ProjectPointerFromName(fSelectedProjectName);
	if (project == nullptr)
		return;

	BString closed(B_TRANSLATE("Project close:"));
	BString name = fSelectedProjectName;

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
	_ProjectOutlineDepopulate(project);
	fProjectObjectList->RemoveItem(project);
	
	//xed
	for (int32 index = 0; index < fEditorObjectList->CountItems(); index++) {
		fEditor = fEditorObjectList->ItemAt(index);
		if (fEditor->GetProject() == project)
		{
			fEditor->SetProject(NULL);
		}
	}
	delete project;

	BString notification;
	notification << closed << " "  << name;
	_SendNotification(notification, "PROJ_CLOSE");
}*/
/*
void
GenioWindow::_ProjectDelete(BString name, bool sourcesToo)
{

	BPath projectPath;

	if ((find_directory(B_USER_SETTINGS_DIRECTORY, &projectPath)) != B_OK) {
		BString text;
		text << B_TRANSLATE("ERROR: Settings directory not found");
		_SendNotification( text.String(), "PROJ_DELETE");
		return;
	}

	// Get base directory for removal
	BString baseDir("");
	for (int32 index = 0; index < fProjectObjectList->CountItems(); index++) {
		Project * project = fProjectObjectList->ItemAt(index);
		if (project->ExtensionedName() == fSelectedProjectName) {
			baseDir.SetTo(project->BasePath());
		}
	}

	_ProjectClose();

	projectPath.Append(GenioNames::kApplicationName);
	projectPath.Append(name);
	BEntry entry(projectPath.Path());

	if (entry.Exists()) {
		BString notification;
		entry.Remove();
		notification << B_TRANSLATE("Project delete:") << "  "  << name.String();
		_SendNotification(notification, "PROJ_DELETE");
	}

	if (sourcesToo == true) {
		if (!baseDir.IsEmpty()) {
			if (_ProjectRemoveDir(baseDir) == B_OK) {
				BString notification;
				notification << B_TRANSLATE("Project delete:") << "  "
					<< B_TRANSLATE("removed") << " " << baseDir;
				_SendNotification(notification, "PROJ_DELETE");
			}
		}
	}
}
*/
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
		 << B_TRANSLATE("Deleting file:")
		 << " \"" << name << "\"" << ".\n\n"
		 << B_TRANSLATE("After deletion file will be lost.") << "\n"
		 << B_TRANSLATE("Do you really want to delete it?") << "\n";

	BAlert* alert = new BAlert(B_TRANSLATE("Delete file dialog"),
		text.String(),
		B_TRANSLATE("Cancel"), B_TRANSLATE("Delete file"), nullptr,
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

	alert->SetShortcut(0, B_ESCAPE);

	int32 choice = alert->Go();

	if (choice == 0)
		return;
	else if (choice == 1) {
		// Close the file if open
		if ((openedIndex = _GetEditorIndex(&ref)) != -1)
			_FileClose(openedIndex, true);

		// Remove the entry
		if (entry.Exists())
			entry.Remove();

		_ProjectFileRemoveItem(false);
	}
}

void
GenioWindow::_ProjectFileExclude()
{
	_ProjectFileRemoveItem(true);
}

/*
 * Full path of a file selected in Project Outline List
 *
 */
BString const
GenioWindow::_ProjectFileFullPath()
{
	ProjectItem* selectedProjectItem = fProjectsFolderBrowser->GetCurrentProjectItem();
	if (selectedProjectItem->GetSourceItem()->Type() == SourceItemType::FileItem)
		return selectedProjectItem->GetSourceItem()->Path();
	else
		return "";
}

void
GenioWindow::_ProjectFileOpen(const BString& filePath)
{
	BEntry entry(filePath);
	entry_ref ref;
	entry.GetRef(&ref);
	BMessage msg;
	msg.AddRef("refs", &ref);
	_FileOpen(&msg, true);
}

//TODO: old function using fProjectsOutline
void
GenioWindow::_ProjectFileRemoveItem(bool addToParseless)
{/*
	TPreferences idmproFile(fSelectedProjectName,
							GenioNames::kApplicationName, 'LOPR');

	int32 index = 0;
	BString superItem(""), file;
	BStringItem *itemType;
	BListItem *item = fSelectedProjectItem, *parent;

	while ((parent = fProjectsOutline->Superitem(item)) != nullptr) {
		itemType = dynamic_cast<BStringItem*>(parent);
		superItem = itemType->Text();
		if (superItem == B_TRANSLATE("Project Files")
			|| superItem == B_TRANSLATE("Project Sources"))
			break;

		item = parent;
	}

	if (superItem == B_TRANSLATE("Project Sources"))
		superItem = "project_source";
	else if (superItem == B_TRANSLATE("Project Files"))
		superItem = "project_file";
	else
		return;

	while ((idmproFile.FindString(superItem, index, &file)) != B_BAD_INDEX) {
		if (_ProjectFileFullPath() == file) {
			idmproFile.RemoveData(superItem, index);
			fProjectsOutline->RemoveItem(fSelectedProjectItem);
			// Excluded items go to "parseless_item" array in settings file so
			// subsequent calls to ProjectParser::ParseProjectFiles() will keep hiding them
			if (addToParseless == true)
				idmproFile.AddString("parseless_item", file);
			break;
		}
		index++;
	}*/
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
GenioWindow::_ProjectFolderNew(BMessage *message)
{

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
	bool findGroupOpen = fFindGroup->IsVisible();

	if (findGroupOpen == false)
		_FindGroupToggled();

	if (!fReplaceGroup->IsVisible()) {
		fReplaceGroup->SetVisible(true);
		fReplaceTextControl->TextView()->Clear();
		// If find group was not open focus and selection go there
		if (findGroupOpen == false)
			_GetFocusAndSelection(fFindTextControl);
		else
			_GetFocusAndSelection(fReplaceTextControl);
	}
	// Replace group was opened, get focus and selection
	else if (fReplaceGroup->IsVisible())
		_GetFocusAndSelection(fReplaceTextControl);
}

void
GenioWindow::_ReplaceGroupToggled()
{
	fReplaceGroup->SetVisible(!fReplaceGroup->IsVisible());

	if (fReplaceGroup->IsVisible()) {
		if (!fFindGroup->IsVisible())
			_FindGroupToggled();
		else if (fFindGroup->IsVisible()) {
			// Find group was already visible, grab focus and selection
			// on replace text control
			_GetFocusAndSelection(fReplaceTextControl);
		}
	}
}

status_t
GenioWindow::_RunInConsole(const BString& command)
{
	status_t status;
	// If no active project go to projects directory
	if (fActiveProject == nullptr)
		chdir(GenioNames::Settings.projects_directory);
	else
		chdir(fActiveProject->Path());

	_ShowLog(kOutputLog);

	BMessage message;
	message.AddString("cmd", command);
	message.AddString("cmd_type", command);

	fConsoleIOThread = new ConsoleIOThread(&message,  BMessenger(this),
		BMessenger(fConsoleIOView));

	status = fConsoleIOThread->Start();

	return status;
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

		fConsoleIOThread = new ConsoleIOThread(&message, BMessenger(this),
			BMessenger(fConsoleIOView));
		fConsoleIOThread->Start();

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

       BRow* fRow = new BRow();
       time_t now =  static_cast<bigtime_t>(real_time_clock());

       fRow->SetField(new BDateField(&now), kTimeColumn);
       fRow->SetField(new BStringField(message), kMessageColumn);
       fRow->SetField(new BStringField(type), kTypeColumn);
       fNotificationsListView->AddRow(fRow, int32(0));
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
	if (active == true) {
		fBuildItem->SetEnabled(true);
		fCleanItem->SetEnabled(true);
		fBuildModeItem->SetEnabled(true);
		fMakeCatkeysItem->SetEnabled(true);
		fMakeBindcatalogsItem->SetEnabled(true);
		fBuildButton->SetEnabled(true);

		// Is this a git project?
		if (fActiveProject->Git())
			fGitMenu->SetEnabled(true);
		else
			fGitMenu->SetEnabled(false);
			
		// Build mode
		bool releaseMode = (fActiveProject->GetBuildMode() == BuildMode::ReleaseMode);
		// Build mode menu
		fBuildModeButton->SetEnabled(!releaseMode);
		fDebugModeItem->SetMarked(!releaseMode);
		fReleaseModeItem->SetMarked(releaseMode);

		// Target exists: enable run button
		chdir(fActiveProject->Path());
		BEntry entry(fActiveProject->GetTarget());
		if (entry.Exists()) {
			fRunItem->SetEnabled(true);
			fRunButton->SetEnabled(true);
			// Enable debug button in debug mode only
			fDebugItem->SetEnabled(!releaseMode);
			fDebugButton->SetEnabled(!releaseMode);

		} else {
			fRunItem->SetEnabled(false);
			fDebugItem->SetEnabled(false);
			fRunButton->SetEnabled(false);
			fDebugButton->SetEnabled(false);
		}
	} else { // here project is inactive
		fBuildItem->SetEnabled(false);
		fCleanItem->SetEnabled(false);
		fRunItem->SetEnabled(false);
		fBuildModeItem->SetEnabled(false);
		fDebugItem->SetEnabled(false);
		fMakeCatkeysItem->SetEnabled(false);
		fMakeBindcatalogsItem->SetEnabled(false);
		fGitMenu->SetEnabled(false);
		fBuildButton->SetEnabled(false);
		fRunButton->SetEnabled(false);
		fDebugButton->SetEnabled(false);
		fBuildModeButton->SetEnabled(false);
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
	fDeleteMenuItem->SetEnabled(editor->CanClear());

	// ToolBar Items
	fUndoButton->SetEnabled(editor->CanUndo());
	fRedoButton->SetEnabled(editor->CanRedo());
	fFileSaveButton->SetEnabled(editor->IsModified());

	// editor is modified by _FilesNeedSave so it should be the last
	// or reload editor pointer
	bool filesNeedSave = _FilesNeedSave();
	fFileSaveAllButton->SetEnabled(filesNeedSave);
	fSaveAllMenuItem->SetEnabled(filesNeedSave);
/*	editor = fEditorObjectList->ItemAt(index);
	// This could be checked too
	if (fTabManager->SelectedTabIndex() != index);
*/
// std::cerr << __PRETTY_FUNCTION__ << " called by: " << caller << " :"<< index << std::endl;

}

/*
 * Status bar is cleaned (both text and trailing) when
 * _UpdateTabChange is called with -1 index
 *
 */
void
GenioWindow::_UpdateStatusBarText(int line, int column)
{
	BString text;
	text << "  " << line << ':' << column;
	fStatusBar->SetText(text.String());
}

/*
 * Index has to be verified before the call
 * so it is not checked here too
 */
void
GenioWindow::_UpdateStatusBarTrailing(int32 index)
{
	Editor* editor = fTabManager->EditorAt(index);

	BString trailing;
	trailing << editor->IsOverwriteString() << '\t';
	trailing << editor->EndOfLineString() << '\t';
	trailing << editor->ModeString() << '\t';
	//trailing << editor->EncodingString() << '\t';

	fStatusBar->SetTrailingText(trailing.String());
}

// Updating menu, toolbar, title.
// Also cleaning Status bar if no open files
void
GenioWindow::_UpdateTabChange(int32 index, const BString& caller)
{
	assert (index >= -1 && index < fTabManager->CountTabs());

	// All files are closed
	if (index == -1) {
		// ToolBar Items
		fFindButton->SetEnabled(false);
		fFindGroup->SetVisible(false);
		fReplaceButton->SetEnabled(false);
		fReplaceGroup->SetVisible(false);
		fFoldButton->SetEnabled(false);
		fUndoButton->SetEnabled(false);
		fRedoButton->SetEnabled(false);
		fFileSaveButton->SetEnabled(false);
		fFileSaveAllButton->SetEnabled(false);
		fFileUnlockedButton->SetEnabled(false);
		fFilePreviousButton->SetEnabled(false);
		fFileNextButton->SetEnabled(false);
		fFileCloseButton->SetEnabled(false);
		fFileMenuButton->SetEnabled(false);

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
		fDeleteMenuItem->SetEnabled(false);
		fSelectAllMenuItem->SetEnabled(false);
		fOverwiteItem->SetEnabled(false);
		fToggleWhiteSpacesItem->SetEnabled(false);
		fToggleLineEndingsItem->SetEnabled(false);
		fLineEndingsMenu->SetEnabled(false);
		fFindItem->SetEnabled(false);
		fReplaceItem->SetEnabled(false);
		fGoToLineItem->SetEnabled(false);
		fBookmarksMenu->SetEnabled(false);

		// Clean Status bar
		fStatusBar->Reset();

		if (GenioNames::Settings.fullpath_title == true)
			SetTitle(GenioNames::kApplicationName);

		return;
	}

	// ToolBar Items
	Editor* editor = fTabManager->EditorAt(index);
	
	fFindButton->SetEnabled(true);
	fReplaceButton->SetEnabled(true);
	fFoldButton->SetEnabled(editor->IsFoldingAvailable());
	fUndoButton->SetEnabled(editor->CanUndo());
	fRedoButton->SetEnabled(editor->CanRedo());
	fFileSaveButton->SetEnabled(editor->IsModified());
	fFileUnlockedButton->SetEnabled(!editor->IsReadOnly());
	fFileCloseButton->SetEnabled(true);
	fFileMenuButton->SetEnabled(true);

	// Arrows
	int32 maxTabIndex = (fTabManager->CountTabs() - 1);
	if (index == 0) {
		fFilePreviousButton->SetEnabled(false);
		if (maxTabIndex > 0)
				fFileNextButton->SetEnabled(true);
	} else if (index == maxTabIndex) {
			fFileNextButton->SetEnabled(false);
			fFilePreviousButton->SetEnabled(true);
	} else {
			fFilePreviousButton->SetEnabled(true);
			fFileNextButton->SetEnabled(true);
	}

	// Menu Items
	fSaveMenuItem->SetEnabled(editor->IsModified());
	fSaveAsMenuItem->SetEnabled(true);
	fCloseMenuItem->SetEnabled(true);
	fCloseAllMenuItem->SetEnabled(true);
	fFoldMenuItem->SetEnabled(editor->IsFoldingAvailable());
	fUndoMenuItem->SetEnabled(editor->CanUndo());
	fRedoMenuItem->SetEnabled(editor->CanRedo());
	fCutMenuItem->SetEnabled(editor->CanCut());
	fCopyMenuItem->SetEnabled(editor->CanCopy());
	fPasteMenuItem->SetEnabled(editor->CanPaste());
	fDeleteMenuItem->SetEnabled(editor->CanClear());
	fSelectAllMenuItem->SetEnabled(true);
	fOverwiteItem->SetEnabled(true);
	// fOverwiteItem->SetMarked(editor->IsOverwrite());
	fToggleWhiteSpacesItem->SetEnabled(true);
	fToggleLineEndingsItem->SetEnabled(true);
	fLineEndingsMenu->SetEnabled(!editor->IsReadOnly());
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
	fFileSaveAllButton->SetEnabled(filesNeedSave);
	fSaveAllMenuItem->SetEnabled(filesNeedSave);



std::cerr << __PRETTY_FUNCTION__ << " called by: " << caller << " :"<< index << std::endl;

}
