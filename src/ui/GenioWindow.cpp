/*
 * Copyright 2022-2023 Nexus6 <nexus6@disroot.org>
 * Copyright 2017..2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GenioWindow.h"

#include <cassert>
#include <string>

#include <Alert.h>
#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <FilePanel.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PathFinder.h>
#include <PopUpMenu.h>
#include <RecentItems.h>
#include <Resources.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <Screen.h>
#include <StringFormat.h>
#include <StringItem.h>

#include "ActionManager.h"
#include "argv_split.h"
#include "ConfigManager.h"
#include "ConfigWindow.h"
#include "ConsoleIOTab.h"
#include "ConsoleIOTabView.h"
#include "EditorMessageFilter.h"
#include "EditorMouseWheelMessageFilter.h"
#include "EditorMessages.h"
#include "EditorTabView.h"
#include "ExtensionManager.h"
#include "FSUtils.h"
#include "FunctionsOutlineView.h"
#include "GenioApp.h"
#include "GenioNamespace.h"
#include "GenioWindowMessages.h"
#include "GitAlert.h"
#include "GitRepository.h"
#include "GlobalStatusView.h"
#include "GoToLineWindow.h"
#include "GSettings.h"
#include "IconMenuItem.h"
#include "JumpNavigator.h"
#include "Languages.h"
#include "Log.h"
#include "LSPEditorWrapper.h"
#include "PanelTabManager.h"
#include "ProblemsPanel.h"
#include "ProjectBrowser.h"
#include "ProjectFolder.h"
#include "ProjectItem.h"
#include "ProjectOpenerWindow.h"
#include "QuitAlert.h"
#include "RemoteProjectWindow.h"
#include "SearchResultTab.h"
#include "SourceControlPanel.h"
#include "SwitchBranchMenu.h"
#include "Task.h"
#include "TemplateManager.h"
#include "TemplatesMenu.h"
#include "TerminalTab.h"
#include "ToolsMenu.h"
#include "Utils.h"


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

static float kDefaultIconSizeSmall = 20.0;
static float kDefaultIconSize = 32.0;

using Genio::Task::Task;


static bool
AcceptsCopyPaste(BView* view)
{
	if (view == nullptr)
		return false;
	if ((view->Parent() != nullptr && dynamic_cast<BScintillaView*>(view->Parent()) != nullptr)
		|| dynamic_cast<BTextView*>(view) != nullptr) {
		return true;
	}
	return false;
}


GenioWindow::GenioWindow(BRect frame)
	:
	BWindow(frame, "Genio", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS |
												B_QUIT_ON_WINDOW_CLOSE |
												B_AUTO_UPDATE_SIZE_LIMITS)
	, fMenuBar(nullptr)
	, fLineEndingsMenu(nullptr)
	, fLineEndingCRLF(nullptr)
	, fLineEndingLF(nullptr)
	, fLineEndingCR(nullptr)
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
	, fSetActiveProjectMenuItem(nullptr)
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
	, fProjectsFolderBrowser(nullptr)
	, fProjectsFolderScroll(nullptr)
	, fActiveProject(nullptr)
	, fTabManager(nullptr)
	, fFindGroup(nullptr)
	, fReplaceGroup(nullptr)
	, fStatusView(nullptr)
	, fFindMenuField(nullptr)
	, fReplaceMenuField(nullptr)
	, fFindTextControl(nullptr)
	, fReplaceTextControl(nullptr)
	, fFindCaseSensitiveCheck(nullptr)
	, fFindWholeWordCheck(nullptr)
	, fFindWrapCheck(nullptr)
	, fRunGroup(nullptr)
	, fRunConsoleProgramText(nullptr)
	, fRunMenuField(nullptr)
	, fConsoleStdinLine("")
	, fOpenPanel(nullptr)
	, fSavePanel(nullptr)
	, fOpenProjectPanel(nullptr)
	, fOpenProjectFolderPanel(nullptr)
	, fImportResourcePanel(nullptr)
	, fProblemsPanel(nullptr)
	, fBuildLogView(nullptr)
	, fMTermView(nullptr)
	, fGoToLineWindow(nullptr)
	, fSearchResultTab(nullptr)
	, fScreenMode(kDefault)
	, fDisableProjectNotifications(false)
	, fPanelTabManager(nullptr)
{
	gMainWindow = this;

	fPanelTabManager = new PanelTabManager();

#ifdef GDEBUG
	fTitlePrefix = ReadFileContent("revision.txt", 16);
#endif

	_InitActions();
	_InitMenu();
	_InitWindow();

	_UpdateTabChange(nullptr, "GenioWindow");

	AddCommonFilter(new KeyDownMessageFilter(MSG_FILE_PREVIOUS_SELECTED, B_LEFT_ARROW,
			B_CONTROL_KEY));
	AddCommonFilter(new KeyDownMessageFilter(MSG_FILE_NEXT_SELECTED, B_RIGHT_ARROW,
			B_CONTROL_KEY));
	AddCommonFilter(new KeyDownMessageFilter(MSG_ESCAPE_KEY, B_ESCAPE, 0, B_DISPATCH_MESSAGE));
	AddCommonFilter(new KeyDownMessageFilter(MSG_TOOLBAR_INVOKED, B_ENTER, 0, B_DISPATCH_MESSAGE));
	AddCommonFilter(new EditorMessageFilter(B_KEY_DOWN, &Editor::BeforeKeyDown));
	AddCommonFilter(new EditorMouseWheelMessageFilter());
	AddCommonFilter(new EditorMessageFilter(B_MOUSE_MOVED, &Editor::BeforeMouseMoved));
	AddCommonFilter(new EditorMessageFilter(B_MODIFIERS_CHANGED, &Editor::BeforeModifiersChanged));
}


void
GenioWindow::Show()
{
	BWindow::Show();

	if (LockLooper()) {
		_ShowPanelTabView(kTabViewLeft,   gCFG["show_projects"], MSG_SHOW_HIDE_LEFT_PANE);
		_ShowPanelTabView(kTabViewRight,  gCFG["show_outline"], MSG_SHOW_HIDE_RIGHT_PANE);
		_ShowPanelTabView(kTabViewBottom, gCFG["show_output"],	MSG_SHOW_HIDE_BOTTOM_PANE);

		_ShowView(fToolBar, gCFG["show_toolbar"],	MSG_TOGGLE_TOOLBAR);
		_ShowView(fStatusView, gCFG["show_statusbar"],	MSG_TOGGLE_STATUSBAR);

		ActionManager::SetPressed(MSG_WHITE_SPACES_TOGGLE, gCFG["show_white_space"]);
		ActionManager::SetPressed(MSG_LINE_ENDINGS_TOGGLE, gCFG["show_line_endings"]);
		ActionManager::SetPressed(MSG_WRAP_LINES, gCFG["wrap_lines"]);

		bool same = ((bool)gCFG["show_white_space"] && (bool)gCFG["show_line_endings"]);
		ActionManager::SetPressed(MSG_TOGGLE_SPACES_ENDINGS, same);

		ActionManager::SetEnabled(MSG_JUMP_GO_BACK, false);
		ActionManager::SetEnabled(MSG_JUMP_GO_FORWARD, false);

		be_app->StartWatching(this, gCFG.UpdateMessageWhat());
		be_app->StartWatching(this, kMsgProjectSettingsUpdated);
		UnlockLooper();
	}

	BMessage message(MSG_PREPARE_WORKSPACE);
	BMessageRunner::StartSending(this, &message, 30000, 1);
}


GenioWindow::~GenioWindow()
{
	delete fOpenPanel;
	delete fSavePanel;
	delete fOpenProjectFolderPanel;
	delete fImportResourcePanel;
	delete fPanelTabManager;
	gMainWindow = nullptr;
}


void
GenioWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PREPARE_WORKSPACE:
			_PrepareWorkspace();
			break;
		case kLSPWorkProgress:
		{
			ProjectFolder* active = GetActiveProject();
			if (active != nullptr && active->Path().Compare(message->GetString("project","")) == 0) {
				SendNotices(MSG_NOTIFY_LSP_INDEXING, message);
			}
			break;
		}
		case MSG_INVOKE_EXTENSION:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK)
				be_roster->Launch(&ref);
			else
				LogError("Can't find entry_ref");
			break;
		}
		case kClassOutline:
		{
			Editor* editor = fTabManager->SelectedEditor();
			if (editor != nullptr)
				editor->GetLSPEditorWrapper()->RequestDocumentSymbols();
			break;
		}
		case kApplyFix:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				Editor* editor = fTabManager->EditorBy(&ref);
				if (editor != nullptr) {
					PostMessage(message, editor);
				}
			}
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code) != B_OK)
				break;
			if (code == gCFG.UpdateMessageWhat()) {
				_HandleConfigurationChanged(message);
			} else if (code == kMsgProjectSettingsUpdated) {
				_HandleProjectConfigurationChanged(message);
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
				_ShowView(fRunGroup, false);
//				fRunConsoleProgramGroup->SetVisible(false);
				fRunConsoleProgramText->MakeFocus(false);
				ActionManager::SetPressed(MSG_RUN_CONSOLE_PROGRAM_SHOW, false);
			}
			break;
		}
		case EDITOR_UPDATE_DIAGNOSTICS:
		{
			editor_id id;
			if (message->FindUInt64(kEditorId, &id) == B_OK) {
				Editor* editor = fTabManager->EditorById(id);
				if (editor != nullptr && editor == fTabManager->SelectedEditor()) {
					fProblemsPanel->UpdateProblems(editor);
				}
			}
			break;
		}
		case EDITOR_UPDATE_SYMBOLS:
		{
			editor_id id;
			if (message->FindUInt64(kEditorId, &id) == B_OK) {
				Editor* editor = fTabManager->EditorById(id);
				if (editor == nullptr)
					return;

				// add the ref also to the external message
				BMessage notifyMessage(MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED);
				notifyMessage.AddMessage("symbols", message);
				notifyMessage.AddUInt64(kEditorId, id);
				if (editor != nullptr) {
					// TODO: Should we call debugger if editor is nullptr here ?
					notifyMessage.AddRef("ref", editor->FileRef());
					notifyMessage.AddInt32("caret_line", editor->GetCurrentLineNumber());
				}
				SendNotices(MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED, &notifyMessage);
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
		{
			// Copied / inspired from Tracker's BContainerWindow
			BView* view = CurrentFocus();
			// Editor is the parent of the ScintillaHaikuView
			if (AcceptsCopyPaste(view)) {
				// (adapted from Tracker's BContainerView::MessageReceived)
				// The selected item is not a BTextView / ScintillaView
				// Since we catch the generic clipboard shortcuts in a way that means
				// the handlers will never get them, we must manually forward them ourselves,
				//
				// However, we have to take care to not forward the custom clipboard messages, else
				// we would wind up in infinite recursion.
				PostMessage(message, view);
			}
			break;
		}
		case B_NODE_MONITOR:
			_HandleNodeMonitorMsg(message);
			break;
		case kCheckEntryRemoved:
			_CheckEntryRemoved(message);
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
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			_FileOpen(message);
			Activate();
			break;
		case B_SAVE_REQUESTED:
			_FileSaveAs(fTabManager->SelectedEditor(), message);
			break;
		case B_UNDO:
		{
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

				fSetActiveProjectMenuItem->SetEnabled(true);

				BMessage noticeMessage(MSG_NOTIFY_BUILDING_PHASE);
				noticeMessage.AddBool("building", false);
				noticeMessage.AddString("cmd_type", cmdType.String());
				noticeMessage.AddString("project_name", message->GetString("project_name"));
				noticeMessage.AddInt32("status", message->GetInt32("status", B_OK));
				SendNotices(MSG_NOTIFY_BUILDING_PHASE, &noticeMessage);

				// TODO: this is not correct: if we start building a project, then
				// change the active project while building, the notification will go to
				// the wrong project (the currently active one)
				// Since we have the project_name here, send the notification
				// to the correct project
				GetActiveProject()->SetBuildingState(false);
			}
			_UpdateProjectActivation(GetActiveProject() != nullptr);
			break;
		}
		case EDITOR_POSITION_CHANGED:
		{
			editor_id id;
			if (message->FindUInt64(kEditorId, &id) == B_OK) {
				Editor* editor = fTabManager->EditorById(id);
				if (editor == fTabManager->SelectedEditor()) {
					// Enable Cut,Copy,Paste shortcuts
					_UpdateSavepointChange(editor, "EDITOR_POSITION_CHANGED");
					//update the OulineView according to the new position.
					fFunctionsOutlineView->SelectSymbolByCaretPosition(message->GetInt32("line", -1));
				}
			}
			break;
		}
		case EDITOR_UPDATE_SAVEPOINT:
		{
			editor_id id;
			bool modified = false;
			if (message->FindUInt64(kEditorId, &id) == B_OK &&
			    message->FindBool("modified", &modified) == B_OK) {

				Editor* editor = fTabManager->EditorById(id);
				if (editor) {
					_UpdateLabel(editor, modified);
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
			GetActiveProject()->SetBuildMode(BuildMode::DebugMode);
			_UpdateProjectActivation(GetActiveProject() != nullptr);
			break;
		}
		case MSG_BUILD_MODE_RELEASE:
		{
			GetActiveProject()->SetBuildMode(BuildMode::ReleaseMode);
			_UpdateProjectActivation(GetActiveProject() != nullptr);
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
		{
			editor_id id = message->GetUInt64(kEditorId, 0);
			Editor* editor = fTabManager->EditorById(id);
			if (!editor)
				editor = fTabManager->SelectedEditor();

			_FileRequestClose(editor);
			break;
		}
		case MSG_FILE_CLOSE_ALL:
			_FileCloseAll();
			break;
		case MSG_FILE_FOLD_TOGGLE:
			_ForwardToSelectedEditor(message);
			break;
		case MSG_COLLAPSE_SYMBOL_NODE:
			_ForwardToSelectedEditor(message);
			break;
		case MSG_FILE_NEXT_SELECTED:
			fTabManager->SelectNext();
			break;
		case MSG_FILE_OPEN:
			fOpenPanel->Show();
			break;
		case MSG_IMPORT_RESOURCE:
			fImportResourcePanel->Show();
			break;
		case MSG_FILE_PREVIOUS_SELECTED:
			fTabManager->SelectPrev();
			break;
		case MSG_FILE_SAVE:
			_FileSave(fTabManager->SelectedEditor());
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
		case MSG_WHEEL_WITH_COMMAND_KEY:
		{
			float deltaX = 0.0f;
			float deltaY = 0.0f;
			message->FindFloat("be:wheel_delta_x", &deltaX);
			message->FindFloat("be:wheel_delta_y", &deltaY);

			if (deltaX == 0.0f && deltaY == 0.0f)
				return;

			if (deltaY == 0.0f)
				deltaY = deltaX;

			int32 zoom = gCFG["editor_zoom"];

			if (deltaY < 0  && zoom < 20) {
				zoom++;
				gCFG["editor_zoom"] = zoom;
			} else if (deltaY > 0 && zoom > -10) {
				zoom--;
				gCFG["editor_zoom"] = zoom;
			}
			break;
		}
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
			_FindMarkAll(message);
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
		case MSG_TOOLBAR_INVOKED:
		{
			if (CurrentFocus() == fFindTextControl->TextView()) {
				const BString& text(fFindTextControl->Text());
				if (fTabManager->SelectedEditor())
					PostMessage(MSG_FIND_NEXT);
				else
					PostMessage(MSG_FIND_IN_FILES);

				fFindTextControl->MakeFocus(true);
			} else if (CurrentFocus() == fRunConsoleProgramText->TextView()) {
				PostMessage(MSG_RUN_CONSOLE_PROGRAM);
			}
			break;
		}
		case MSG_FIND_NEXT:
		{
			_FindNext(message, false);
			break;
		}
		case MSG_FIND_PREVIOUS:
		{
			_FindNext(message, true);
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
				BString project_path = message->GetString("project_path", GetActiveProject()->Path().String());
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
			if (fGoToLineWindow == nullptr) {
				fGoToLineWindow = new GoToLineWindow(this);
			}
			fGoToLineWindow->ShowCentered(Frame());
			break;
		case MSG_WHITE_SPACES_TOGGLE:
			gCFG["show_white_space"] = !gCFG["show_white_space"];
			break;
		case MSG_LINE_ENDINGS_TOGGLE:
			gCFG["show_line_endings"] = !gCFG["show_line_endings"];
			break;
		case MSG_TOGGLE_SPACES_ENDINGS:
			gCFG["show_line_endings"] = !ActionManager::IsPressed(MSG_TOGGLE_SPACES_ENDINGS);
			gCFG["show_white_space"]  = !ActionManager::IsPressed(MSG_TOGGLE_SPACES_ENDINGS);
			break;
		case MSG_WRAP_LINES:
			gCFG["wrap_lines"] = !gCFG["wrap_lines"];
			break;
		case MSG_DUPLICATE_LINE:
		case MSG_DELETE_LINES:
		case MSG_COMMENT_SELECTED_LINES:
		case MSG_FILE_TRIM_TRAILING_SPACE:
		case MSG_AUTOCOMPLETION:
		case MSG_FORMAT:
		case MSG_GOTODEFINITION:
		case MSG_GOTODECLARATION:
		case MSG_GOTOIMPLEMENTATION:
		case MSG_RENAME:
		case MSG_SWITCHSOURCE:
		case MSG_LOAD_RESOURCE:
			_ForwardToSelectedEditor(message);
			break;
		case MSG_FIND_IN_BROWSER:
		{
			editor_id id = message->GetUInt64(kEditorId, 0);
			Editor*	editor = fTabManager->EditorById(id);
			if (editor == nullptr)
				editor = fTabManager->SelectedEditor();

			if (editor == nullptr || editor->GetProjectFolder() == nullptr)
				return;

			GetProjectBrowser()->SelectItemByRef(editor->GetProjectFolder(), *editor->FileRef());
			break;
		}
		case MSG_MAKE_BINDCATALOGS:
			_MakeBindcatalogs();
			break;
		case MSG_MAKE_CATKEYS:
			_MakeCatkeys();
			break;
		case MSG_PROJECT_CLOSE:
			_ProjectFolderClose(GetActiveProject());
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
			entry_ref templateRef;
			if (message->FindRef("template_ref", &templateRef) != B_OK) {
				LogError("Invalid template %s", templateRef.name);
				break;
			}
			entry_ref destRef;
			if (message->FindRef("directory", &destRef) != B_OK) {
				LogError("Invalid destination directory %s", destRef.name);
				break;
			}
			BString name;
			if (message->FindString("name", &name) != B_OK) {
				LogError("Invalid destination name %s", name.String());
				break;
			}
			if (TemplateManager::CopyProjectTemplate(&templateRef, &destRef, name.String()) == B_OK) {
				BPath destPath(&destRef);
				destPath.Append(name.String());
				BEntry destEntry(destPath.Path());
				entry_ref destination;
				destEntry.GetRef(&destination);
				_ProjectFolderOpen(destination);
			} else {
				LogError("TemplateManager: could create %s from %s to %s",
							name.String(), templateRef.name, destRef.name);
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
				break;
			}

			if (type == "new_folder")
				_TemplateNewFolder(message);
			else if (type == "new_folder_template") {
				// new_folder_template corresponds to creating a new project
				// there is no need to check the selected item in the ProjectBrowser
				// A FilePanel is shown to let the user select the destination of the new project
				_TemplateNewProject(message);
			} else if (type ==  "new_file_template") {
				// new_file_template corresponds to creating a new file
				_TemplateNewFile(message);
			}
			break;
		}
		case MSG_PROJECT_MENU_CLOSE: {
			ProjectFolder* project = (ProjectFolder*)message->GetPointer("project",
									fProjectsFolderBrowser->GetProjectFromSelectedItem());
			_ProjectFolderClose(project);
			break;
		}
		case MSG_PROJECT_MENU_OPEN_FILE:
		{
			_FileOpen(message);
			break;
		}
		case MSG_PROJECT_MENU_DELETE_FILE:
			_ProjectFileDelete(message);
			break;
		case MSG_PROJECT_MENU_RENAME_FILE:
			_ProjectRenameFile(message);
			break;
		case MSG_PROJECT_MENU_SET_ACTIVE:
		{
			ProjectFolder* project = (ProjectFolder*)message->GetPointer("project", nullptr);
			if (project != nullptr)
				_ProjectFolderActivate(project);
			break;
		}
		case MSG_PROJECT_MENU_SHOW_IN_TRACKER:
		{
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				_ShowItemInTracker(&ref);
				return;
			}
			break;
		}
		case MSG_PROJECT_MENU_OPEN_TERMINAL:
		{
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK) {
				_OpenTerminalWorkingDirectory(&ref);
				return;
			}
			break;
		}
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
			ProjectFolder* project = (ProjectFolder*)message->GetPointer("project", GetActiveProject());
			if (project != nullptr) {
				ConfigWindow* window = new ConfigWindow(project->Settings(), false);
				BString windowTitle(B_TRANSLATE("Project:"));
				windowTitle.Append(" ");
				windowTitle.Append(project->Name());
				window->SetTitle(windowTitle.String());
				window->Show();
			}
			break;
		}
		case MSG_PROJECT_FOLDER_OPEN:
			_ProjectFolderOpen(message);
			break;
		case MSG_PROJECT_OPEN_INITIATED:
		{
			ProjectFolder* project = (ProjectFolder*)message->GetPointer("project", nullptr);
			bool activate = message->GetBool("activate", false);
			entry_ref ref;
			message->FindRef("ref", &ref);
			_ProjectFolderOpenInitiated(project, ref, activate);
			break;
		}
		case MSG_PROJECT_OPEN_ABORTED:
		{
			ProjectFolder* project = (ProjectFolder*)message->GetPointer("project", nullptr);
			bool activate = message->GetBool("activate", false);
			entry_ref ref;
			message->FindRef("ref", &ref);
			_ProjectFolderOpenAborted(project, ref, activate);
			break;
		}
		case MSG_PROJECT_OPEN_COMPLETED:
		{
			ProjectFolder* project = (ProjectFolder*)message->GetPointer("project", nullptr);
			bool activate = message->GetBool("activate", false);
			entry_ref ref;
			message->FindRef("ref", &ref);
			_ProjectFolderOpenCompleted(project, ref, activate);
			break;
		}
		case MSG_REPLACE_GROUP_SHOW:
			_ReplaceGroupShow(true);
			break;
		case MSG_REPLACE_ALL:
			_Replace(message, REPLACE_ALL);
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
			_Replace(message, REPLACE_NEXT);
			break;
		case MSG_REPLACE_ONE:
			_Replace(message, REPLACE_ONE);
			break;
		case MSG_REPLACE_PREVIOUS:
			_Replace(message, REPLACE_PREVIOUS);
			break;
		case MSG_RUN_CONSOLE_PROGRAM_SHOW:
		{
			_ShowView(fRunGroup, fRunGroup->IsHidden());
			fRunConsoleProgramText->MakeFocus(!fRunGroup->IsHidden());
			/*if (fRunConsoleProgramGroup->IsVisible()) {
				fRunConsoleProgramGroup->SetVisible(false);
				fRunConsoleProgramText->MakeFocus(false);
			} else {
				fRunConsoleProgramGroup->SetVisible(true);
				fRunConsoleProgramText->MakeFocus(true);
			}*/
			ActionManager::SetPressed(MSG_RUN_CONSOLE_PROGRAM_SHOW, !fRunGroup->IsHidden());
			break;
		}
		case MSG_RUN_CONSOLE_PROGRAM:
		{
			BString command = message->GetString("command", fRunConsoleProgramText->Text());
			if (!command.IsEmpty())
				_RunInConsole(command);
			if (!message->HasString("command"))
				fRunConsoleProgramText->SetText("");
			break;
		}
		case MSG_RUN_TARGET:
			_RunTarget();
			break;
		case MSG_SHOW_HIDE_LEFT_PANE:
			gCFG["show_projects"] = !bool(gCFG["show_projects"]);
			break;
		case MSG_SHOW_HIDE_RIGHT_PANE:
			gCFG["show_outline"] = !bool(gCFG["show_outline"]);
			break;
		case MSG_SHOW_HIDE_BOTTOM_PANE:
			gCFG["show_output"] = !bool(gCFG["show_output"]);
			break;
		case MSG_TOGGLE_TOOLBAR:
			gCFG["show_toolbar"] = !bool(gCFG["show_toolbar"]);
			break;
		case MSG_TOGGLE_STATUSBAR:
			gCFG["show_statusbar"] = !bool(gCFG["show_statusbar"]);
			break;
		case MSG_FULLSCREEN:
		case MSG_FOCUS_MODE:
			_ToogleScreenMode(message->what);
			break;
		case MSG_TEXT_OVERWRITE:
			_ForwardToSelectedEditor(message);
			break;
		case MSG_WINDOW_SETTINGS:
		{
			ConfigWindow* window = new ConfigWindow(gCFG);
			window->Show();
			break;
		}
		case MSG_RELOAD_EDITORCONFIG:
		{
			for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
				Editor* editor = fTabManager->EditorAt(index);
				editor->LoadEditorConfig();
				editor->ApplySettings();
			}
			break;
		}
		case EditorTabView::kETVSelectedTab:
		{
			editor_id id;
			if (message->FindUInt64(kEditorId, &id) == B_OK) {
				Editor* editor = fTabManager->EditorById(id);
				if (editor == nullptr) {
					LogError("Selecting editor but it's null! (index %d)", index);
					break;
				}
				const int32 be_line   = message->GetInt32("start:line", -1);
				const int32 lsp_char  = message->GetInt32("start:character", -1);

				if (lsp_char >= 0 && be_line > -1) {
					editor->GoToLSPPosition(be_line - 1, lsp_char);
				} else if (be_line > -1) {
					editor->GoToLine(be_line);
				}

				editor->GrabFocus();
				_UpdateTabChange(editor, "EditorTabView::kETVSelectedTab");
				FakeMouseMovement(editor);

				BMessage tabSelectedNotice(MSG_NOTIFY_EDITOR_FILE_SELECTED);
				tabSelectedNotice.AddPointer("project", editor->GetProjectFolder());
				tabSelectedNotice.AddRef("ref", editor->FileRef());
				SendNotices(MSG_NOTIFY_EDITOR_FILE_SELECTED, &tabSelectedNotice);

				// TODO: when closing then reopening a tab, the message will be empty
				// because the symbols BMessage returned by GetDocumentSymbols()
				// is empty as the Editor has just been created
				BMessage symbolsChanged(MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED);
				BMessage symbols;
				editor->GetDocumentSymbols(&symbols);
				symbolsChanged.AddUInt64(kEditorId, id);
				symbolsChanged.AddRef("ref", editor->FileRef());
				symbolsChanged.AddMessage("symbols", &symbols);
				symbolsChanged.AddInt32("caret_line", editor->GetCurrentLineNumber());
				SendNotices(MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED, &symbolsChanged);
			}
			break;
		}
		case MSG_FILE_CLOSE_OTHER:
		{
			std::vector<Editor*> editors;
			editor_id id = message->GetUInt64(kEditorId, 0);
			fTabManager->ForEachEditor([&](Editor* editor) {
				if (editor->Id() != id) {
					editors.push_back(editor);
				}
				return true;
			});

			_CloseMultipleTabs(editors);
			break;
		}
		case EditorTabView::kETVCloseTab:
		{
			editor_id id = message->GetUInt64(kEditorId, 0);
			Editor*	editor = fTabManager->EditorById(id);
			if (editor == nullptr)
				return;
			std::vector<Editor*> editors = { editor };
			_CloseMultipleTabs(editors);
			break;
		}
		case EditorTabView::kETVNewTab:
		{
			editor_id id = message->GetUInt64(kEditorId, 0);
			if (id != 0) {
				Editor* editor = fTabManager->EditorById(id);
				if (editor != nullptr) {
					if (message->GetBool("caret_position", false) == true) {
						editor->SetSavedCaretPosition();
					}
					ProjectFolder* project = editor->GetProjectFolder();
					if (project != nullptr) {
						fTabManager->SetTabColor(editor, project->Color());
					}
				}
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
		case MSG_HELP_DOCS:
			_ShowDocumentation();
			break;
		case kMsgCapabilitiesUpdated:
			_UpdateTabChange(fTabManager->SelectedEditor(), "kMsgCapabilitiesUpdated");
			break;
		case MSG_JUMP_GO_BACK:
			JumpNavigator::getInstance()->JumpToPrev();
			break;
		case MSG_JUMP_GO_FORWARD:
			JumpNavigator::getInstance()->JumpToNext();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


/* virtual */
void
GenioWindow::MenusBeginning()
{
	BWindow::MenusBeginning();

	// Build "Set active" menu
	for (int32 p = 0; p < GetProjectBrowser()->CountProjects(); p++) {
		ProjectFolder* project = GetProjectBrowser()->ProjectAt(p);
		BMessage* setActiveMessage = new BMessage(MSG_PROJECT_MENU_SET_ACTIVE);
		setActiveMessage->AddPointer("project", project);
		BMenuItem* item = new BMenuItem(project->Name(), setActiveMessage);
		if (project->Active())
			item->SetEnabled(false);
		fSetActiveProjectMenuItem->AddItem(item);
	}

	BView* view = CurrentFocus();
	if (view == nullptr)
		return;

	BScintillaView* scintilla = nullptr;
	BTextView* textView = nullptr;
	if (view->Parent() != nullptr &&
		(scintilla = dynamic_cast<BScintillaView*>(view->Parent())) != nullptr) {
			ActionManager::SetEnabled(B_CUT,   CanScintillaViewCut(scintilla));
			ActionManager::SetEnabled(B_COPY,  CanScintillaViewCopy(scintilla));
			ActionManager::SetEnabled(B_PASTE, CanScintillaViewPaste(scintilla));
	} else if ((textView = (dynamic_cast<BTextView*>(view))) != nullptr) {
			int32 start;
			int32 finish;
			textView->GetSelection(&start, &finish);
			bool canEdit = textView->IsEditable();

			ActionManager::SetEnabled(B_CUT,   canEdit && start != finish);
			ActionManager::SetEnabled(B_COPY,  start != finish);
			ActionManager::SetEnabled(B_PASTE, canEdit && be_clipboard->SystemCount() > 0);

	} else {
			ActionManager::SetEnabled(B_CUT,   false);
			ActionManager::SetEnabled(B_COPY,  false);
			ActionManager::SetEnabled(B_PASTE, false);
	}
}


/* virtual */
void
GenioWindow::MenusEnded()
{
	BWindow::MenusEnded();
	fSetActiveProjectMenuItem->RemoveItems(0, fSetActiveProjectMenuItem->CountItems(), true);
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


ProjectBrowser*
GenioWindow::GetProjectBrowser() const
{
	return fProjectsFolderBrowser;
}


EditorTabView*
GenioWindow::TabManager() const
{
	return fTabManager;
}


void
GenioWindow::_PrepareWorkspace()
{
	// Load workspace - reopen projects

	BMessage started(MSG_NOTIFY_WORKSPACE_PREPARATION_STARTED);
	SendNotices(MSG_NOTIFY_WORKSPACE_PREPARATION_STARTED, &started);

	// Disable MSG_NOTIFY_PROJECT_SET_ACTIVE and MSG_NOTIFY_PROJECT_LIST_CHANGE while we populate
	// the workspace
	// TODO: improve how projects are loaded and notices are sent over
	if (gCFG["reopen_projects"]) {
		fDisableProjectNotifications = true;
		GSettings projects(GenioNames::kSettingsProjectsToReopen, 'PRRE');
		status_t status = B_OK;
		if (!projects.IsEmpty()) {
			BString projectName;
			BString activeProject = projects.GetString("active_project");
			for (auto count = 0; projects.FindString("project_to_reopen",
										count, &projectName) == B_OK; count++) {
				entry_ref ref;
				status = get_ref_for_path(projectName, &ref);
				if (status == B_OK) {
					status = _ProjectFolderOpen(ref, projectName == activeProject);
				}
			}
		}
		if (GetActiveProject() != nullptr)
			GetProjectBrowser()->SelectProjectAndScroll(GetActiveProject());

		fDisableProjectNotifications = false;
		if (status == B_OK) {
			SendNotices(MSG_NOTIFY_PROJECT_LIST_CHANGED);
			BMessage noticeMessage(MSG_NOTIFY_PROJECT_SET_ACTIVE);
			const ProjectFolder* activeProject = GetActiveProject();
			if (activeProject != nullptr) {
				noticeMessage.AddString("active_project_name", activeProject->Name());
				noticeMessage.AddString("active_project_path", activeProject->Path());
			}
			SendNotices(MSG_NOTIFY_PROJECT_SET_ACTIVE, &noticeMessage);
		}
	}

	// Reopen files
	if (gCFG["reopen_files"]) {
		GSettings files(GenioNames::kSettingsFilesToReopen, 'FIRE');
		if (!files.IsEmpty()) {
			entry_ref ref;
			int32 index = -1;
			BMessage message(B_REFS_RECEIVED);

			if (files.FindInt32("opened_index", &index) == B_OK) {
				message.AddInt32("opened_index", index);
				int32 count;
				for (count = 0; files.FindRef("file_to_reopen", count, &ref) == B_OK; count++)
					message.AddRef("refs", &ref);
				// Found an index and found some files, post message
				if (index > -1 && count > 0)
					PostMessage(&message);
			}
		}
	}

	BMessage completed(MSG_NOTIFY_WORKSPACE_PREPARATION_COMPLETED);
	SendNotices(MSG_NOTIFY_WORKSPACE_PREPARATION_COMPLETED, &completed);
}


//Freely inspired by the haiku Terminal fullscreen function.
void
GenioWindow::_ToogleScreenMode(int32 action)
{
	if (fScreenMode == kDefault) { // go fullscreen
		fScreenModeSettings.MakeEmpty();
		fScreenModeSettings["saved_frame"] = Frame();
		fScreenModeSettings["saved_look"] = (int32)Look();
		fScreenModeSettings["show_projects"] = fPanelTabManager->IsPanelTabViewVisible(kTabViewLeft);//!fProjectsTabView->IsHidden();
		fScreenModeSettings["show_output"]   = fPanelTabManager->IsPanelTabViewVisible(kTabViewBottom);//!fOutputTabView->IsHidden();
		fScreenModeSettings["show_toolbar"]  = !fToolBar->IsHidden();
		fScreenModeSettings["show_outline"]  = fPanelTabManager->IsPanelTabViewVisible(kTabViewRight); //!fRightTabView->IsHidden();

		BScreen screen(this);
		fMenuBar->Hide();
		SetLook(B_NO_BORDER_WINDOW_LOOK);
		ResizeTo(screen.Frame().Width() + 1, screen.Frame().Height() + 1);
		MoveTo(screen.Frame().left, screen.Frame().top);
		SetFlags(Flags() | (B_NOT_RESIZABLE | B_NOT_MOVABLE));

		ActionManager::SetEnabled(MSG_TOGGLE_TOOLBAR, false);
		ActionManager::SetEnabled(MSG_SHOW_HIDE_LEFT_PANE, false);
		ActionManager::SetEnabled(MSG_SHOW_HIDE_RIGHT_PANE, false);
		ActionManager::SetEnabled(MSG_SHOW_HIDE_BOTTOM_PANE, false);

		if (action == MSG_FULLSCREEN) {
			fScreenMode = kFullscreen;
		} else if (action == MSG_FOCUS_MODE) {
			_ShowView(fToolBar,         false, MSG_TOGGLE_TOOLBAR);
			_ShowPanelTabView(kTabViewLeft,   false, MSG_SHOW_HIDE_LEFT_PANE);
			_ShowPanelTabView(kTabViewRight,  false, MSG_SHOW_HIDE_RIGHT_PANE);
			_ShowPanelTabView(kTabViewBottom, false, MSG_SHOW_HIDE_BOTTOM_PANE);
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
		ActionManager::SetEnabled(MSG_SHOW_HIDE_LEFT_PANE, true);
		ActionManager::SetEnabled(MSG_SHOW_HIDE_RIGHT_PANE, true);
		ActionManager::SetEnabled(MSG_SHOW_HIDE_BOTTOM_PANE, true);

		_ShowView(fToolBar,         fScreenModeSettings["show_toolbar"], MSG_TOGGLE_TOOLBAR);

		_ShowPanelTabView(kTabViewLeft,   fScreenModeSettings["show_projects"], MSG_SHOW_HIDE_LEFT_PANE);
		_ShowPanelTabView(kTabViewRight,  fScreenModeSettings["show_outline"], MSG_SHOW_HIDE_RIGHT_PANE);
		_ShowPanelTabView(kTabViewBottom, fScreenModeSettings["show_output"],	MSG_SHOW_HIDE_BOTTOM_PANE);

		fScreenMode = kDefault;
	}
}


void
GenioWindow::_ForwardToSelectedEditor(BMessage* msg)
{
	Editor* editor = fTabManager->SelectedEditor();
	if (editor != nullptr) {
		PostMessage(msg, editor);
	}
}


void
GenioWindow::_CloseMultipleTabs(std::vector<Editor*>& editors)
{
	std::vector<Editor*> unsavedEditor;
	for(Editor* editor:editors) {
		if (editor->IsModified())
			unsavedEditor.push_back(editor);
	}

	if (!_FileRequestSaveList(unsavedEditor))
		return;

	for(Editor* editor:editors) {
		_RemoveTab(editor);
	}
}


bool
GenioWindow::_FileRequestSaveAllModified()
{
	std::vector<Editor*> unsavedEditor;
	fTabManager->ForEachEditor([&](Editor* editor){
		if (editor->IsModified())
			unsavedEditor.push_back(editor);

		return true;
	});
	return _FileRequestSaveList(unsavedEditor);
}


bool
GenioWindow::_FileRequestClose(Editor* editor)
{
	if (editor != nullptr) {
		if (editor->IsModified()) {
			std::vector<Editor*> unsavedEditor { editor };
			if (!_FileRequestSaveList(unsavedEditor))
				return false;

		}
		_RemoveTab(editor);
	}

	return true;
}


bool
GenioWindow::_FileRequestSaveList(std::vector<Editor*>& unsavedEditor)
{
	if (unsavedEditor.empty())
		return true;

	if (unsavedEditor.size() == 1) {
		Editor* editor = unsavedEditor[0];
		BString text(B_TRANSLATE("Save changes to file \"%file%\""));
		text.ReplaceAll("%file%", editor->FilePath().String());

		BAlert* alert = new BAlert("CloseAndSaveDialog", text,
 			B_TRANSLATE("Cancel"), B_TRANSLATE("Don't save"), B_TRANSLATE("Save"),
 			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

		alert->SetShortcut(0, B_ESCAPE);

		switch (alert->Go()) {
			case 2: // Save and close.
				_FileSave(editor);
			case 1: // Don't save (close)
				return true;
			case 0: // Cancel
			default:
				return false;
		};
	}

	std::vector<std::string> unsavedPaths;
	for (Editor* editor:unsavedEditor) {
		unsavedPaths.push_back(std::string(editor->FilePath().String()));
	}

	//Let's use Koder QuitAlert!

	QuitAlert* quitAlert = new QuitAlert(unsavedPaths);
	auto filesToSave = quitAlert->Go();

	if (filesToSave.empty()) //Cancel the request!
		return false;

	auto bter = filesToSave.begin();
	auto iter = unsavedEditor.begin();
	while (iter != unsavedEditor.end()) {
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

	gCFG["show_projects"] = fPanelTabManager->IsPanelTabViewVisible(kTabViewLeft);
	gCFG["show_output"]   = fPanelTabManager->IsPanelTabViewVisible(kTabViewBottom);
	gCFG["show_toolbar"]  = !fToolBar->IsHidden();

	BMessage tabview_config;
	fPanelTabManager->SaveConfiguration(tabview_config);
	gCFG["tabviews"] = tabview_config;

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


Editor*
GenioWindow::_AddEditorTab(entry_ref* ref, BMessage* addInfo)
{
	Editor* editor = new Editor(ref, BMessenger(this));
	fTabManager->AddEditor(ref->name, editor, addInfo);
	return editor;
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
	return _DoBuildOrCleanProject("build");
}


status_t
GenioWindow::_CleanProject()
{
	return _DoBuildOrCleanProject("clean");
}


status_t
GenioWindow::_DoBuildOrCleanProject(const BString& cmd)
{
	// Should not happen
	if (GetActiveProject() == nullptr)
		return B_ERROR;

	const BString projectName = GetActiveProject()->Name();
	BString command;
	if (cmd == "build")
		command	<< GetActiveProject()->GetBuildCommand();
	else if (cmd == "clean")
		command	<< GetActiveProject()->GetCleanCommand();
	if (command.IsEmpty()) {
		LogInfoF("Empty %s command for project [%s]", cmd.String(), projectName.String());

		BString message;
		message << "No " << cmd << " command found!\n"
				   "Please configure the project to provide\n"
				   "a valid " << cmd << " configuration.";
		return _AlertInvalidBuildConfig(message);
	}

	_UpdateProjectActivation(false);

	fBuildLogView->Clear();
	if (gCFG["show_build_panel"])
		_ShowOutputTab(kTabBuildLog);

	LogInfoF("%s started: [%s]", cmd.String(), projectName.String());

	// TODO: Disable the Set Active item while building, at least for now.
	// various parts of the code refer to fActiveProject to send build state notifications
	// and it's wrong if we allow switching active project while building
	fSetActiveProjectMenuItem->SetEnabled(false);

	BMessage noticeMessage(MSG_NOTIFY_BUILDING_PHASE);
	noticeMessage.AddBool("building", true);
	noticeMessage.AddString("cmd_type", cmd.String());
	noticeMessage.AddString("project_name", projectName);
	SendNotices(MSG_NOTIFY_BUILDING_PHASE, &noticeMessage);

	GetActiveProject()->SetBuildingState(true);

	BString claim;
	claim << cmd << " ";
	claim << projectName;
	claim << " (";
	claim << (GetActiveProject()->GetBuildMode() == BuildMode::ReleaseMode ? B_TRANSLATE("Release") : B_TRANSLATE("Debug"));
	claim << ")";

	GMessage message = {{"cmd", command},
						{"cmd_type", cmd.String()},
						{"project_name", projectName},
						{"banner_claim", claim }};

	// Go to appropriate directory
	chdir(GetActiveProject()->Path());
	auto buildPath = GetActiveProject()->GetBuildFilePath();
	if (!buildPath.IsEmpty())
		chdir(buildPath);

	return fBuildLogView->RunCommand(&message);
}


status_t
GenioWindow::_DebugProject()
{
	// Should not happen
	if (GetActiveProject() == nullptr)
		return B_ERROR;

	// Release mode enabled, should not happen
	// TODO: why not ? One could want to debug a project
	// regardless to which build profile he chose
	if (GetActiveProject()->GetBuildMode() == BuildMode::ReleaseMode)
		return B_ERROR;

	argv_split parser(GetActiveProject()->GetTarget().String());
	parser.parse(GetActiveProject()->GetExecuteArgs().String());

	return be_roster->Launch("application/x-vnd.Haiku-debugger",
		parser.getArguments().size() , parser.argv());
}


status_t
GenioWindow::_RemoveTab(Editor* editor)
{
	if (!editor)
		return B_ERROR;
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

	fTabManager->RemoveEditor(editor);
	editor = nullptr;

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

	fTabManager->ReverseForEachEditor([&](Editor* editor) {
		if (!editor->IsModified())
			_RemoveTab(editor);

		return true;
	});
}


status_t
GenioWindow::_FileOpenAtStartup(BMessage* msg)
{
	int32 opened_index = msg->GetInt32("opened_index", 0);
	entry_ref ref;
	int32 refsCount = 0;
	while (msg->FindRef("refs", refsCount++, &ref) == B_OK) {
		_FileOpenWithPosition(&ref, false, -1,-1);
	}
	if (fTabManager->CountTabs() > opened_index) {
		fTabManager->SelectTab(opened_index);
	}
	return B_OK;
}


status_t
GenioWindow::_FileOpen(BMessage* msg)
{
	//start-up files.
	if (msg->HasInt32("opened_index")) {
		return _FileOpenAtStartup(msg);
	}

	entry_ref firstAdded = {0, 0, 0};
	entry_ref ref;
	int32 refsCount = 0;
	while (msg->FindRef("refs", refsCount++, &ref) == B_OK) {
		if (BEntry(&ref).IsDirectory()) {
			_ProjectFolderOpen(msg);
			continue;
		}

		const int32 be_line   = msg->GetInt32("start:line", msg->GetInt32("be:line", -1));
		const int32 lsp_char  = msg->GetInt32("start:character", -1);
		const bool openWithPreferred = msg->GetBool("openWithPreferred", false);

		Editor* editor = fTabManager->EditorBy(&ref);
		if (editor != nullptr) {
			_SelectEditorToPosition(editor, be_line, lsp_char);
		} else {
			if(_FileOpenWithPosition(&ref , openWithPreferred, be_line, lsp_char) != B_OK)
				continue;
		}

		_ApplyEditsToSelectedEditor(msg);
		if (firstAdded.directory == 0)
			firstAdded = ref;

		if (refsCount == 1){
			entry_ref fromRef;
			if (msg->FindRef("jumpFrom", 0, &fromRef) == B_OK) {
				JumpNavigator::getInstance()->JumpingTo(ref, fromRef);
			}
		}
	}

	ActionManager::SetEnabled(MSG_JUMP_GO_BACK, JumpNavigator::getInstance()->HasPrev());
	ActionManager::SetEnabled(MSG_JUMP_GO_FORWARD, JumpNavigator::getInstance()->HasNext());


	if (firstAdded.directory != 0) {
		fTabManager->SelectTab(&firstAdded);
	}

	// reply to Editor create scripting
	BMessage reply(B_REPLY);
	reply.AddRef("result", &firstAdded);
	msg->SendReply(&reply);

	return B_OK;
}


void
GenioWindow::_ApplyEditsToSelectedEditor(BMessage* msg)
{
	// apply LSP edits
	std::string edit = msg->GetString("edit", "");
	if (!edit.empty())
		fTabManager->SelectedEditor()->ApplyEdit(edit);
}


status_t
GenioWindow::_SelectEditorToPosition(Editor* editor, int32 be_line, int32 lsp_char)
{
	GMessage selectTabInfo = {{"start:line", be_line},{"start:character", lsp_char}};
	Editor* selected = fTabManager->SelectedEditor();

	if (editor != selected) {
		fTabManager->SelectTab(editor->FileRef(), &selectTabInfo);
	} else {
		if (lsp_char >= 0 && be_line > -1) {
			selected->GoToLSPPosition(be_line - 1, lsp_char);
		} else if (be_line > -1) {
			selected->GoToLine(be_line);
		}
		selected->GrabFocus();
	}
	return B_OK;
}


status_t
GenioWindow::_FileOpenWithPosition(entry_ref* ref, bool openWithPreferred, int32 be_line, int32 lsp_char)
{
	if (!BEntry(ref).Exists())
		return B_ERROR;

	if (!_FileIsSupported(ref)) {
		if (openWithPreferred)
			_FileOpenWithPreferredApp(ref); // TODO: make this optional?
		return B_ERROR;
	}

	//this will force getting the caret position from file attributes when loaded.
	GMessage selectTabInfo = {{ "caret_position", true }, {"start:line", be_line},{"start:character", lsp_char}};

	Editor* editor = _AddEditorTab(ref, &selectTabInfo);

	if (editor == nullptr) {
		LogError("Failed adding editor");
		return B_ERROR;
	}

	status_t status = editor->LoadFromFile();
	if (status != B_OK) {
		LogError("Failed loading file: %s", ::strerror(status));
		return status;
	}

	// register the file as a recent one.
	be_roster->AddToRecentDocuments(ref, GenioNames::kApplicationSignature);

	// Select the newly added tab
	fTabManager->SelectTab(editor->FileRef(), &selectTabInfo);

	// TODO: Move some other stuff into _PostFileLoad()
	_PostFileLoad(editor);

	LogInfo("File open: %s", editor->Name().String());
	return B_OK;
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
GenioWindow::_FileSave(Editor* editor)
{
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

	status_t saveStatus = editor->SaveToFile();

	// Restart monitoring
	editor->StartMonitoring();

	if (saveStatus == B_OK)
		LogInfoF("File saved! (%s)", editor->FilePath().String());
	else
		LogErrorF("Error saving file! (%s): ", editor->FilePath().String(), ::strerror(saveStatus));

	_PostFileSave(editor);

	return B_OK;
}


void
GenioWindow::_FileSaveAll(ProjectFolder* onlyThisProject)
{
	const int32 filesCount = fTabManager->CountTabs();
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
			_FileSave(editor);
	}
}


status_t
GenioWindow::_FileSaveAs(Editor* editor, BMessage* message)
{
	if (editor == nullptr) {
		LogError("_FileSaveAs: NULL editor pointer" );
		return B_ERROR;
	}

	entry_ref ref;
	status_t status;
	if ((status = message->FindRef("directory", &ref)) != B_OK)
		return status;
	BString name;
	if ((status = message->FindString("name", &name)) != B_OK)
		return status;

	BPath path(&ref);
	path.Append(name);
	BEntry entry(path.Path(), true);
	entry_ref newRef;
	if ((status = entry.GetRef(&newRef)) != B_OK)
		return status;

	editor->SetFileRef(&newRef);
	fTabManager->SetTabLabel(editor, editor->Name().String());

	/* Modified files 'Saved as' get saved to an unmodified state.
	 * It should be cool to take the modified state to the new file and let
	 * user choose to save or discard modifications. Left as a TODO.
	 * In case do not forget to update label
	 */
	//_UpdateLabel(selection, editor->IsModified());

	_FileSave(editor);

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
GenioWindow::_PreFileLoad(Editor* editor)
{
}


void
GenioWindow::_PostFileLoad(Editor* editor)
{
	if (editor != nullptr) {
		// Assign the right project to the Editor
		for (int32 cycleIndex = 0; cycleIndex < GetProjectBrowser()->CountProjects(); cycleIndex++) {
			ProjectFolder* project = GetProjectBrowser()->ProjectAt(cycleIndex);
			_TryAssociateEditorWithProject(editor, project);
		}
		editor->ApplySettings();
		editor->GetLSPEditorWrapper()->RequestDocumentSymbols();
		BMessage noticeMessage(MSG_NOTIFY_EDITOR_FILE_OPENED);
		noticeMessage.AddString("file_name", editor->FilePath());
		SendNotices(MSG_NOTIFY_EDITOR_FILE_OPENED, &noticeMessage);
	}
}


void
GenioWindow::_PreFileSave(Editor* editor)
{
	LogTrace("GenioWindow::_PreFileSave(%s)", editor->FilePath().String());

	editor->TrimTrailingWhitespace();
}


void
GenioWindow::_PostFileSave(Editor* editor)
{
	LogTrace("GenioWindow::_PostFileSave(%s)", editor->FilePath().String());

	// TODO: Also handle cases where the file is saved from outside Genio ?
	ProjectFolder* project = editor->GetProjectFolder();
	if (gCFG["build_on_save"] &&
		project != nullptr && project == GetActiveProject()) {
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


void
GenioWindow::_FindMarkAll(BMessage* message)
{
	BString textToFind(fFindTextControl->Text());
	if (textToFind.IsEmpty())
		return;

	_AddSearchFlags(message);
	message->SetString("text", textToFind);
	message->SetBool("wrap", fFindWrapCheck->Value());
	_ForwardToSelectedEditor(message);
	_UpdateFindMenuItems(textToFind);

}


void
GenioWindow::_AddSearchFlags(BMessage* msg)
{
	msg->SetBool("match_case", fFindCaseSensitiveCheck->Value());
	msg->SetBool("whole_word", fFindWholeWordCheck->Value());
}


void
GenioWindow::_FindNext(BMessage* message, bool backwards)
{
	BString textToFind(fFindTextControl->Text());
	if (textToFind.IsEmpty())
		return;

	_AddSearchFlags(message);
	message->SetString("text", textToFind);
	message->SetBool("wrap", fFindWrapCheck->Value());
	message->SetBool("backward", backwards);
	_ForwardToSelectedEditor(message);
	_UpdateFindMenuItems(textToFind);
}


void
GenioWindow::_FindInFiles()
{
	if (!GetActiveProject())
		return;

	BString text(fFindTextControl->Text());
	if (text.IsEmpty())
		return;

	fSearchResultTab->SetAndStartSearch(text, (bool)fFindWholeWordCheck->Value(),
											  (bool)fFindCaseSensitiveCheck->Value(),
											  GetActiveProject());
	_ShowOutputTab(kTabSearchResult);
	_UpdateFindMenuItems(fFindTextControl->Text());
}


void
GenioWindow::_GetFocusAndSelection(BTextControl* control) const
{
	control->MakeFocus(true);
	// If some text is selected, use that
	Editor* editor = fTabManager->SelectedEditor();
	if (editor) {
		BString selection = editor->Selection();
		if (selection.IsEmpty() == false) {
			control->SetText(selection.String());
		}
	}
}


status_t
GenioWindow::_Git(const BString& git_command)
{
	// Should not happen
	if (GetActiveProject() == nullptr)
		return B_ERROR;

	// Pretend building or running
	_UpdateProjectActivation(false);

	//fConsoleIOView->Clear();
	_ShowOutputTab(kTabOutputLog);

	BString command;
	command	<< "git " << git_command;

	BMessage message;
	message.AddString("cmd", command);
	message.AddString("cmd_type", command);
	message.AddString("project_name", GetActiveProject()->Name());

	// Go to appropriate directory
	chdir(GetActiveProject()->Path());

	return fMTermView->RunCommand(&message);
}


void
GenioWindow::_HandleExternalMoveModification(entry_ref* oldRef, entry_ref* newRef)
{
	Editor* editor = fTabManager->EditorBy(oldRef);
	if (editor == nullptr) {
		LogError("_HandleExternalMoveModification: Invalid move file: oldRef doesn't exist");
		return;
	}

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

 	int32 choice = alert->Go();

	if (choice == 0)
		return;
	else if (choice == 1) {
		_FileRequestClose(editor);
	} else if (choice == 2) {
		editor->SetFileRef(newRef);
		fTabManager->SetTabLabel(editor, editor->Name().String());
		_UpdateLabel(editor, editor->IsModified());

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
GenioWindow::_HandleExternalRemoveModification(Editor* editor)
{
	if (editor == nullptr) {
		return; //TODO notify
	}

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
			_FileSave(editor);
		return;
	} else if (choice == 1) {
		_RemoveTab(editor);

		BString notification;
		notification << "File info: " << fileName << " removed externally";
		LogInfo(notification.String());
	}
}


void
GenioWindow::_HandleExternalStatModification(Editor* editor)
{
	if (editor == nullptr)
		return;

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
GenioWindow::_CheckEntryRemoved(BMessage *msg)
{
	node_ref nref;
	BString name;
	int64 dir;

	if (msg->FindInt32("device", &nref.device) != B_OK
		|| msg->FindString("name", &name) != B_OK
		|| msg->FindInt64("directory", &dir) != B_OK
		|| msg->FindInt64("node", &nref.node) != B_OK)
			return;


	// Let's check if the path exists, if the file is loaded in Genio
	// and if the current genio node_ref is the same as the one of the path received.

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
					Editor* editor = fTabManager->EditorBy(&nref);
					if (editor == nullptr) {
						return;
					}
					node_ref newNode;
					if (entry.GetNodeRef(&newNode) == B_OK) {
						// same path but different node_ref
						// it means someone is removing the old
						// and moving over another file.
						if ( newNode != nref) {
							entry_ref xref;
							entry.GetRef(&xref);
							editor->StopMonitoring();
							editor->SetFileRef(&xref);
							editor->StartMonitoring();
							_HandleExternalStatModification(editor);
						}
						// else a B_STAT_CHANGED is notifyng the change.
					}
					return;
				}
			}
		}
	}

	// the file is gone for sure!
	_HandleExternalRemoveModification(fTabManager->EditorBy(&nref));
}


void
GenioWindow::_HandleNodeMonitorMsg(BMessage* msg)
{
	// TODO: Move this away from here
	int32 opcode;
	status_t status;
	if ((status = msg->FindInt32("opcode", &opcode)) != B_OK) {
		LogError("Node monitor message without an opcode!");
		msg->PrintToStream();
		return;
	}

	switch (opcode) {
		case B_ENTRY_MOVED: {
			int32 device;
			int64 srcDir;
			int64 dstDir;
			BString name;
			BString oldName;

			if (msg->FindInt32("device", &device) != B_OK
				|| msg->FindInt64("to directory", &dstDir) != B_OK
				|| msg->FindInt64("from directory", &srcDir) != B_OK
				|| msg->FindString("name", &name) != B_OK
				|| msg->FindString("from name", &oldName) != B_OK)
					break;

			entry_ref oldRef(device, srcDir, oldName.String());
			entry_ref newRef(device, dstDir, name.String());

			_HandleExternalMoveModification(&oldRef, &newRef);

			break;
		}
		case B_ENTRY_REMOVED: {
			// This message can be triggered by different use cases

			// Some git commands generate a B_ENTRY_REMOVED
			// even if the file is just restored from a previous version.
			// (same entry_ref, same node_ref)

			// Some other command (for example haiku-format)
			// delete the file, and then move another file to the old path.
			// (same entry_ref, different node_ref)

			// Of course he could be a real file removed (deleted).

			// Let's deferr the decision to some milliseconds to have a better understanding
			// of what's going on.

			msg->what = kCheckEntryRemoved;
			BMessageRunner::StartSending(this, msg, 1500, 1);
		}
		break;
		case B_STAT_CHANGED: {
			node_ref nref;
			int32 fields;

			if (msg->FindInt32("device", &nref.device) != B_OK
				|| msg->FindInt64("node", &nref.node) != B_OK
				|| msg->FindInt32("fields", &fields) != B_OK)
					break;

			if ((fields & (B_STAT_MODE | B_STAT_UID | B_STAT_GID)) != 0)
				break; 			// TODO recheck permissions

			if (((fields & B_STAT_MODIFICATION_TIME)  != 0)
			// Do not reload if the file just got touched
				&& ((fields & B_STAT_ACCESS_TIME)  == 0)) {
				_HandleExternalStatModification(fTabManager->EditorBy(&nref));
			}

			break;
		}
		default:
			break;
	}
}

void
GenioWindow::_InitCommandRunToolbar()
{
	fRunGroup = new ToolBar(this);
	if (gCFG["use_small_icons"]) {
		fRunGroup->ChangeIconSize(be_control_look->ComposeIconSize(kDefaultIconSizeSmall).Width());
	} else {
		fRunGroup->ChangeIconSize(be_control_look->ComposeIconSize(kDefaultIconSize).Width());
	}

	fRunMenuField = new BMenuField("RecentMenuField", NULL, new BMenu(B_TRANSLATE("Run:")));
	fRunMenuField->SetExplicitMaxSize(BSize(kFindReplaceOPSize, B_SIZE_UNSET));
	fRunMenuField->SetExplicitMinSize(BSize(kFindReplaceOPSize, B_SIZE_UNSET));

	fRunConsoleProgramText = new BTextControl("RunTextControl", "" , "", nullptr);
	fRunConsoleProgramText->TextView()->SetMaxBytes(kFindReplaceMaxBytes*2);
	float charWidth = fRunConsoleProgramText->StringWidth("0", 1);
	fRunConsoleProgramText->SetExplicitMinSize(
						BSize(charWidth * kFindReplaceMinBytes + 10.0f,
						B_SIZE_UNSET));
	BSize doubleSize = fRunConsoleProgramText->MinSize();
	doubleSize.width *= 2;
	fRunConsoleProgramText->SetExplicitMaxSize(doubleSize);

	fRunGroup->AddView(BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
												.Add(fRunMenuField)
												.Add(fRunConsoleProgramText).View());

	fRunGroup->AddAction(MSG_RUN_CONSOLE_PROGRAM, "", "kIconRun",  false);
	fRunGroup->FindButton(MSG_RUN_CONSOLE_PROGRAM)->SetLabel(B_TRANSLATE("Run"));
	fRunGroup->AddGlue();
	fRunGroup->Hide();

	// Update run command working directory tooltip too
	BString tooltip("cwd: ");
	tooltip << (const char*)gCFG["projects_directory"];
	fRunConsoleProgramText->SetToolTip(tooltip);

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


	fFindCaseSensitiveCheck = new BCheckBox(B_TRANSLATE_COMMENT("Match case", "Short as possible."),
		new BMessage(MSG_FIND_MATCH_CASE));
	fFindWholeWordCheck = new BCheckBox(B_TRANSLATE_COMMENT("Whole word", "Short as possible."),
		new BMessage(MSG_FIND_WHOLE_WORD));
	fFindWrapCheck = new BCheckBox(B_TRANSLATE_COMMENT("Wrap around",
		"Continue searching from the beginning when reaching the end of the file. Short as possible."),
		new BMessage(MSG_FIND_WRAP));

	fFindWrapCheck->SetValue(gCFG["find_wrap"] ? B_CONTROL_ON : B_CONTROL_OFF);
	fFindWholeWordCheck->SetValue(gCFG["find_whole_word"] ? B_CONTROL_ON : B_CONTROL_OFF);
	fFindCaseSensitiveCheck->SetValue(gCFG["find_match_case"] ? B_CONTROL_ON : B_CONTROL_OFF);

	fFindGroup = new ToolBar(this);
	if (gCFG["use_small_icons"]) {
		fFindGroup->ChangeIconSize(be_control_look->ComposeIconSize(kDefaultIconSizeSmall).Width());
	} else {
		fFindGroup->ChangeIconSize(be_control_look->ComposeIconSize(kDefaultIconSize).Width());
	}
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
	if (gCFG["use_small_icons"]) {
		fReplaceGroup->ChangeIconSize(be_control_look->ComposeIconSize(kDefaultIconSizeSmall).Width());
	} else {
		fReplaceGroup->ChangeIconSize(be_control_look->ComposeIconSize(kDefaultIconSize).Width());
	}
	fReplaceGroup->AddView(BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
												.Add(fReplaceMenuField)
												.Add(fReplaceTextControl).View());
	fReplaceGroup->AddAction(MSG_REPLACE_ONE, B_TRANSLATE("Replace selection"), "kIconReplaceOne");
	fReplaceGroup->AddAction(MSG_REPLACE_NEXT, B_TRANSLATE("Replace and find next"), "kIconReplaceNext");
	fReplaceGroup->AddAction(MSG_REPLACE_PREVIOUS, B_TRANSLATE("Replace and find previous"), "kIconReplacePrev");
	fReplaceGroup->AddAction(MSG_REPLACE_ALL, B_TRANSLATE("Replace all"), "kIconReplaceAll");
	fReplaceGroup->AddGlue();
	fReplaceGroup->Hide();

	_InitCommandRunToolbar();

	// Editor tab & view

	fTabManager = new EditorTabView(BMessenger(this));

	fEditorTabsGroup = BLayoutBuilder::Group<>(B_VERTICAL, 0.0f)
		.Add(fRunGroup)
		.Add(fFindGroup)
		.Add(fReplaceGroup)
		.Add(fTabManager)
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
GenioWindow::_ShowPanelTabView(const char* tabview_name, bool show, int32 msgWhat)
{
	fPanelTabManager->ShowPanelTabView(tabview_name, show);
	if (msgWhat > -1)
		ActionManager::SetPressed(msgWhat, fPanelTabManager->IsPanelTabViewVisible(tabview_name));
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

	ActionManager::RegisterAction(MSG_FILE_CLOSE_OTHER,
									B_TRANSLATE("Close other"));

	ActionManager::RegisterAction(MSG_IMPORT_RESOURCE,
									B_TRANSLATE("Import as RDEF array" B_UTF8_ELLIPSIS));

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
									"", "");
	ActionManager::RegisterAction(MSG_FILE_FOLD_TOGGLE,
									B_TRANSLATE("Fold/Unfold all"),
									B_TRANSLATE("Fold/Unfold all"),
									"kIconFold_4");
	ActionManager::RegisterAction(MSG_WHITE_SPACES_TOGGLE,
									B_TRANSLATE("Show whitespace"),
									B_TRANSLATE("Show whitespace"), "");
	ActionManager::RegisterAction(MSG_LINE_ENDINGS_TOGGLE,
									B_TRANSLATE("Show line endings"),
									B_TRANSLATE("Show line endings"), "");
	ActionManager::RegisterAction(MSG_TOGGLE_SPACES_ENDINGS,
									B_TRANSLATE("Show whitespace and line endings"),
									B_TRANSLATE("Show whitespace and line endings"),
									"kIconShowPunctuation");
	ActionManager::RegisterAction(MSG_WRAP_LINES,
									B_TRANSLATE("Wrap lines"),
									B_TRANSLATE("Wrap lines"), "kIconWrapLines");

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

	ActionManager::RegisterAction(MSG_RENAME,
									B_TRANSLATE("Rename symbol" B_UTF8_ELLIPSIS));

	ActionManager::RegisterAction(MSG_FIND_IN_BROWSER,
									B_TRANSLATE("Show in projects browser"), "", "", 'Y');

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
									B_TRANSLATE("Close active project"),
									"", "", 'C', B_OPTION_KEY);

	ActionManager::RegisterAction(MSG_RUN_CONSOLE_PROGRAM_SHOW,
									B_TRANSLATE("Run console program"),
									B_TRANSLATE("Run console program"),
									"kIconTerminal");

	// TODO: Should we call those  left/right panes ?
	ActionManager::RegisterAction(MSG_SHOW_HIDE_LEFT_PANE,
									B_TRANSLATE("Show left pane"),
									B_TRANSLATE("Show/Hide left pane"),
									"kIconWinNav");

	ActionManager::RegisterAction(MSG_SHOW_HIDE_RIGHT_PANE,
									B_TRANSLATE("Show right pane"),
									B_TRANSLATE("Show/Hide right pane"),
									"kIconWinOutline");

	ActionManager::RegisterAction(MSG_SHOW_HIDE_BOTTOM_PANE,
									B_TRANSLATE("Show bottom pane"),
									B_TRANSLATE("Show/Hide bottom pane"),
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

	ActionManager::RegisterAction(MSG_TOGGLE_STATUSBAR,
									B_TRANSLATE("Show statusbar"));

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

	ActionManager::RegisterAction(MSG_RELOAD_EDITORCONFIG,
									B_TRANSLATE("Reload .editorconfig"),
									B_TRANSLATE("Reload .editorconfig"),
									"");

	ActionManager::RegisterAction(MSG_BUFFER_LOCK,
									B_TRANSLATE("Read-only"),
									B_TRANSLATE("Make file read-only"), "kIconLocked");

	// TODO: we use ALT+N (where N is 1-9) to switch tab (like Web+), and CTRL+LEFT/RIGHT to switch
	// to previous/next. Too bad ALT+LEFT/RIGHT are already taken. Maybe we should change to
	// CTRL+N (but should be changed in Web+, too)
	ActionManager::RegisterAction(MSG_FILE_PREVIOUS_SELECTED, B_TRANSLATE("Go to previous tab"),
									B_TRANSLATE("Go to previous tab"), "",
									B_LEFT_ARROW, B_CONTROL_KEY);

	ActionManager::RegisterAction(MSG_FILE_NEXT_SELECTED, B_TRANSLATE("Go to next tab"),
									B_TRANSLATE("Go to next tab"), "",
									B_RIGHT_ARROW, B_CONTROL_KEY);

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


	ActionManager::RegisterAction(MSG_PROJECT_MENU_SHOW_IN_TRACKER,
									B_TRANSLATE("Show in Tracker"),
									B_TRANSLATE("Show in Tracker"));

	ActionManager::RegisterAction(MSG_PROJECT_MENU_OPEN_TERMINAL,
									B_TRANSLATE("Open in Terminal"),
									B_TRANSLATE("Open in Terminal"));


	ActionManager::RegisterAction(MSG_JUMP_GO_FORWARD, B_TRANSLATE("Go forward"),
									B_TRANSLATE("Go forward"), "kIconForward_2", B_RIGHT_ARROW,
									B_SHIFT_KEY|B_COMMAND_KEY);

	ActionManager::RegisterAction(MSG_JUMP_GO_BACK, B_TRANSLATE("Go back"),
									B_TRANSLATE("Go back"), "kIconBack_1", B_LEFT_ARROW,
									B_SHIFT_KEY|B_COMMAND_KEY);

}


void
GenioWindow::_InitMenu()
{
	// Menu
	fMenuBar = new BMenuBar("menubar");

	BMenu* appMenu = new BMenu("");
	appMenu->AddItem(new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED)));
	appMenu->AddItem(new BMenuItem(B_TRANSLATE("Help" B_UTF8_ELLIPSIS),
		new BMessage(MSG_HELP_DOCS)));
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

	//ActionManager::AddItem(MSG_FILE_NEW, fileMenu);

	fileMenu->AddItem(fFileNewMenuItem = new TemplatesMenu(this, B_TRANSLATE("New"),
			new BMessage(MSG_FILE_NEW), new BMessage(MSG_SHOW_TEMPLATE_USER_FOLDER),
			TemplateManager::GetDefaultTemplateDirectory(),
			TemplateManager::GetUserTemplateDirectory(),
			TemplatesMenu::SHOW_ALL_VIEW_MODE,	true));

	ActionManager::AddItem(MSG_FILE_OPEN, fileMenu);

	fileMenu->AddItem(new BMenuItem(BRecentFilesList::NewFileListMenu(
			B_TRANSLATE("Open recent" B_UTF8_ELLIPSIS), nullptr, nullptr, this,
			kRecentFilesNumber, true, nullptr, GenioNames::kApplicationSignature), nullptr));
	ActionManager::AddItem(MSG_IMPORT_RESOURCE, fileMenu);

	fileMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_FILE_SAVE,     fileMenu);
	ActionManager::AddItem(MSG_FILE_SAVE_AS,  fileMenu);
	ActionManager::AddItem(MSG_FILE_SAVE_ALL, fileMenu);

	fileMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_FILE_CLOSE,     fileMenu);
	ActionManager::AddItem(MSG_FILE_CLOSE_ALL, fileMenu);
	ActionManager::AddItem(MSG_FILE_CLOSE_OTHER, fileMenu);
	fileMenu->AddSeparatorItem();
	ActionManager::AddItem(MSG_FIND_IN_BROWSER, fileMenu);

	ActionManager::SetEnabled(MSG_FILE_NEW, false);
	ActionManager::SetEnabled(MSG_IMPORT_RESOURCE, false);
	ActionManager::SetEnabled(MSG_FILE_SAVE, false);
	ActionManager::SetEnabled(MSG_FILE_SAVE_AS, false);
	ActionManager::SetEnabled(MSG_FILE_SAVE_ALL, false);
	ActionManager::SetEnabled(MSG_FILE_CLOSE, false);
	ActionManager::SetEnabled(MSG_FILE_CLOSE_ALL, false);
	ActionManager::SetEnabled(MSG_FILE_CLOSE_OTHER, false);

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
	fLineEndingsMenu->AddItem((fLineEndingLF = new BMenuItem(B_TRANSLATE("LF (Haiku, Unix, macOS)"),
		new BMessage(MSG_EOL_CONVERT_TO_UNIX))));
	fLineEndingsMenu->AddItem((fLineEndingCRLF = new BMenuItem(B_TRANSLATE("CRLF (Windows, Dos)"),
		new BMessage(MSG_EOL_CONVERT_TO_DOS))));
	fLineEndingsMenu->AddItem((fLineEndingCR = new BMenuItem(B_TRANSLATE("CR (Classic Mac OS)"),
		new BMessage(MSG_EOL_CONVERT_TO_MAC))));

	ActionManager::SetEnabled(B_UNDO, false);
	ActionManager::SetEnabled(B_REDO, false);

	ActionManager::SetEnabled(B_CUT, false);
	ActionManager::SetEnabled(B_COPY, false);
	ActionManager::SetEnabled(B_PASTE, false);
	ActionManager::SetEnabled(B_SELECT_ALL, false);
	ActionManager::SetEnabled(MSG_TEXT_OVERWRITE, false);
	ActionManager::SetPressed(MSG_TEXT_OVERWRITE, false);
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
	ActionManager::AddItem(MSG_WRAP_LINES, viewMenu);
	ActionManager::AddItem(MSG_SWITCHSOURCE, viewMenu);
	ActionManager::SetEnabled(MSG_FILE_FOLD_TOGGLE, false);
	ActionManager::SetEnabled(MSG_WHITE_SPACES_TOGGLE, false);
	ActionManager::SetEnabled(MSG_LINE_ENDINGS_TOGGLE, false);
	ActionManager::SetEnabled(MSG_WRAP_LINES, false);
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

	searchMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_JUMP_GO_BACK, searchMenu);
	ActionManager::AddItem(MSG_JUMP_GO_FORWARD, searchMenu);

	ActionManager::SetEnabled(MSG_GOTODEFINITION, false);
	ActionManager::SetEnabled(MSG_GOTODECLARATION, false);
	ActionManager::SetEnabled(MSG_GOTOIMPLEMENTATION, false);
	ActionManager::SetEnabled(MSG_FIND_IN_BROWSER, false);
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

	fSetActiveProjectMenuItem = new BMenu(B_TRANSLATE("Set active"));
	projectMenu->AddItem(fSetActiveProjectMenuItem);

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
	ActionManager::AddItem(MSG_RELOAD_EDITORCONFIG, projectMenu);
	ActionManager::SetEnabled(MSG_RELOAD_EDITORCONFIG, true);

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
	ActionManager::AddItem(MSG_SHOW_HIDE_LEFT_PANE, submenu);
	ActionManager::AddItem(MSG_SHOW_HIDE_RIGHT_PANE, submenu);
	ActionManager::AddItem(MSG_SHOW_HIDE_BOTTOM_PANE, submenu);
	ActionManager::AddItem(MSG_TOGGLE_TOOLBAR, submenu);
	ActionManager::AddItem(MSG_TOGGLE_STATUSBAR, submenu);
	windowMenu->AddItem(submenu);

	windowMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_FILE_PREVIOUS_SELECTED, windowMenu);
	ActionManager::AddItem(MSG_FILE_NEXT_SELECTED, windowMenu);

	windowMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_FULLSCREEN, windowMenu);
	ActionManager::AddItem(MSG_FOCUS_MODE, windowMenu);
	fMenuBar->AddItem(windowMenu);

	auto app = reinterpret_cast<GenioApp *>(be_app);
	if (app->GetExtensionManager()->GetExtensions().size() > 0) {
		auto toolsMenu = new ToolsMenu(B_TRANSLATE("Tools"), MSG_INVOKE_EXTENSION, this);
		fMenuBar->AddItem(toolsMenu);
	}
}


BMenu*
GenioWindow::_CreateLanguagesMenu()
{
	fLanguageMenu = new BMenu(B_TRANSLATE("Language"));

	for (size_t i = 0; i < Languages::GetCount(); ++i) {
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
	if (gCFG["use_small_icons"]) {
		fToolBar->ChangeIconSize(be_control_look->ComposeIconSize(kDefaultIconSizeSmall).Width());
	} else {
		fToolBar->ChangeIconSize(be_control_look->ComposeIconSize(kDefaultIconSize).Width());
	}
	ActionManager::AddItem(MSG_SHOW_HIDE_LEFT_PANE, fToolBar);
	ActionManager::AddItem(MSG_SHOW_HIDE_RIGHT_PANE, fToolBar);
	ActionManager::AddItem(MSG_SHOW_HIDE_BOTTOM_PANE, fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_FILE_FOLD_TOGGLE, fToolBar);
	ActionManager::AddItem(B_UNDO, fToolBar);
	ActionManager::AddItem(B_REDO, fToolBar);
	ActionManager::AddItem(MSG_FILE_SAVE, fToolBar);
	ActionManager::AddItem(MSG_FILE_SAVE_ALL, fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_TOGGLE_SPACES_ENDINGS, fToolBar);
	ActionManager::AddItem(MSG_WRAP_LINES, fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_BUILD_PROJECT, fToolBar);
	ActionManager::AddItem(MSG_CLEAN_PROJECT, fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_RUN_TARGET, fToolBar);
	ActionManager::AddItem(MSG_DEBUG_PROJECT, fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_FIND_GROUP_TOGGLED,		fToolBar);
	ActionManager::AddItem(MSG_REPLACE_GROUP_TOGGLED,	fToolBar);
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_RUN_CONSOLE_PROGRAM_SHOW, fToolBar);
	ActionManager::AddItem(MSG_JUMP_GO_BACK, fToolBar);
	ActionManager::AddItem(MSG_JUMP_GO_FORWARD, fToolBar);


	fToolBar->AddGlue();

	ActionManager::AddItem(MSG_BUFFER_LOCK, fToolBar);

#if 0
	// Useless icons here
	fToolBar->AddSeparator();

	ActionManager::AddItem(MSG_FILE_PREVIOUS_SELECTED, fToolBar);
	ActionManager::AddItem(MSG_FILE_NEXT_SELECTED, fToolBar);
	ActionManager::AddItem(MSG_FILE_CLOSE, fToolBar);

	fToolBar->AddAction(MSG_FILE_MENU_SHOW, B_TRANSLATE("Open files list"), "kIconFileList");
#endif
	ActionManager::SetEnabled(MSG_FIND_GROUP_TOGGLED, true);
	ActionManager::SetEnabled(MSG_REPLACE_GROUP_TOGGLED, true);
}


void
GenioWindow::_InitTabViews()
{
	BMessage cfg = gCFG["tabviews"];
	fPanelTabManager->LoadConfiguration(cfg);
	fPanelTabManager->CreatePanelTabView(kTabViewBottom, 	B_HORIZONTAL);
	fPanelTabManager->CreatePanelTabView(kTabViewLeft,		B_VERTICAL);
	fPanelTabManager->CreatePanelTabView(kTabViewRight, 	B_VERTICAL);

	//Bottom
	fProblemsPanel = new ProblemsPanel(fPanelTabManager, kTabProblems);

	fBuildLogView = new ConsoleIOTabView(B_TRANSLATE("Build log"), BMessenger(this));
	fMTermView 	  = new ConsoleIOTabView(B_TRANSLATE("Console I/O"), BMessenger(this));

	fSearchResultTab = new SearchResultTab(fPanelTabManager, kTabSearchResult);
	fTerminalTab	= new TerminalTab();

	fPanelTabManager->AddPanelByConfig(fProblemsPanel, kTabProblems);
	fPanelTabManager->AddPanelByConfig(fBuildLogView, kTabBuildLog);
	fPanelTabManager->AddPanelByConfig(fMTermView, kTabOutputLog);
	fPanelTabManager->AddPanelByConfig(fSearchResultTab, kTabSearchResult);
	fPanelTabManager->AddPanelByConfig(fTerminalTab, kTabTerminal);


	//LEFT
	fProjectsFolderBrowser = new ProjectBrowser();
	fSourceControlPanel = new SourceControlPanel();
	fPanelTabManager->AddPanelByConfig(fProjectsFolderBrowser, kTabProjectBrowser);
	fPanelTabManager->AddPanelByConfig(fSourceControlPanel, kTabSourceControl);

	//RIGHT
	fFunctionsOutlineView = new FunctionsOutlineView();
	fPanelTabManager->AddPanelByConfig(fFunctionsOutlineView, kTabOutlineView);
}


void
GenioWindow::_InitWindow()
{
	_InitToolbar();
	_InitTabViews();
	_InitCentralSplit();

	// Layout
	fRootLayout = BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(fMenuBar)
		.Add(fToolBar)

		.AddSplit(B_VERTICAL, 0.0f) // output split
		.SetInsets(-2.0f, 0.0f, -2.0f, -2.0f)
			.AddSplit(B_HORIZONTAL, 0.0f) // sidebar split
				.Add(fPanelTabManager->GetPanelTabView(kTabViewLeft), kProjectsWeight)
				.Add(fEditorTabsGroup, kEditorWeight)  // Editor
				.Add(fPanelTabManager->GetPanelTabView(kTabViewRight), 1)
			.End() // sidebar split
			.Add(fPanelTabManager->GetPanelTabView(kTabViewBottom), kOutputWeight)
		.End() //  output split
		.Add(fStatusView = new GlobalStatusView())
	;

	// Panels
	const char* projectsDirectory = gCFG["projects_directory"];
	const BEntry entry(projectsDirectory, true);
	entry_ref ref;
	entry.GetRef(&ref);

	fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), &ref, B_FILE_NODE, true);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), &ref, B_FILE_NODE, false);

	fImportResourcePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), nullptr, B_FILE_NODE, true,
		new BMessage(MSG_LOAD_RESOURCE));

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
	if (GetActiveProject() == nullptr)
		return;

	fBuildLogView->Clear();
	_ShowOutputTab(kTabBuildLog);

	// TODO: this only works for makefile_engine based projects
	BMessage message;
	if (GetActiveProject()->GetBuildMode() == BuildMode::DebugMode)
		message.AddString("cmd", "DEBUGGER=TRUE make bindcatalogs");
	else
		message.AddString("cmd", "make bindcatalogs");
	message.AddString("cmd_type", "bindcatalogs");
	message.AddString("project_name", GetActiveProject()->Name());

	// Go to appropriate directory
	chdir(GetActiveProject()->Path());
	auto buildPath = GetActiveProject()->GetBuildFilePath();
	if (!buildPath.IsEmpty())
		chdir(buildPath);

	fBuildLogView->RunCommand(&message);
}


void
GenioWindow::_MakeCatkeys()
{
	// Should not happen
	if (GetActiveProject() == nullptr)
		return;

	fBuildLogView->Clear();
	_ShowOutputTab(kTabBuildLog);

	BMessage message;
	message.AddString("cmd", "make catkeys");
	message.AddString("cmd_type", "catkeys");
	message.AddString("project_name", GetActiveProject()->Name());

	// Go to appropriate directory
	chdir(GetActiveProject()->Path());
	auto buildPath = GetActiveProject()->GetBuildFilePath();
	if (!buildPath.IsEmpty())
		chdir(buildPath);

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
	// There is no active project
	if (GetActiveProject() == nullptr) {
		if (project != nullptr) {
			SetActiveProject(project);
			project->SetActive(true);
			_UpdateProjectActivation(true);
		}
	} else {
		// There was an active project already
		GetActiveProject()->SetActive(false);
		if (project != nullptr) {
			SetActiveProject(project);
			project->SetActive(true);
			_UpdateProjectActivation(true);
		}
	}

	if (!fDisableProjectNotifications) {
		BMessage noticeMessage(MSG_NOTIFY_PROJECT_SET_ACTIVE);
		const ProjectFolder* activeProject = GetActiveProject();
		if (activeProject != nullptr) {
			noticeMessage.AddString("active_project_name", activeProject->Name());
			noticeMessage.AddString("active_project_path", activeProject->Path());
		}
		SendNotices(MSG_NOTIFY_PROJECT_SET_ACTIVE, &noticeMessage);
	}

	// Update run command working directory tooltip too
	BString tooltip;
	tooltip << "cwd: " << GetActiveProject()->Path();
	fRunConsoleProgramText->SetToolTip(tooltip);
}


void
GenioWindow::_TryAssociateEditorWithProject(Editor* editor, ProjectFolder* project)
{
	// let's check if editor belongs to this project
	BString projectPath = project->Path().String();
	projectPath = projectPath.Append("/");
	LogTrace("Open project [%s] vs editor project [%s]",
		projectPath.String(), editor->FilePath().String());
	if (editor->GetProjectFolder() == nullptr) {
		// TODO: This isn't perfect: if we open a subfolder of
		// an existing project as new project, the two would clash
		if (editor->FilePath().StartsWith(projectPath))
			editor->SetProjectFolder(project);
	}
}


void
GenioWindow::_ProjectFileDelete(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("ref", &ref) != B_OK)
		return;
	BEntry entry(&ref);
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
			_RemoveTab(fTabManager->EditorBy(&ref));
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
GenioWindow::_ProjectRenameFile(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("ref", &ref) == B_OK) {
		const ProjectFolder* project = reinterpret_cast<const ProjectFolder*>(message->GetPointer("project", nullptr));
		if (project != nullptr) {
			ProjectItem *item = GetProjectBrowser()->GetItemByRef(project, ref);
			GetProjectBrowser()->InitRename(item);
		}
	}
}


void
GenioWindow::_TemplateNewFile(BMessage* message)
{
	entry_ref source;
	entry_ref dest;
	if (message->FindRef("sender_ref", &dest) != B_OK) {
		LogError("Can't find sender_ref in message!");
		return;
	}

	if (message->FindRef("refs", &source) != B_OK) {
		LogError("Can't find refs in message!");
		return;
	}
	entry_ref refNew;
	status_t status = TemplateManager::CopyFileTemplate(&source, &dest, &refNew);
	if (status != B_OK) {
		OKAlert(B_TRANSLATE("New file"),
				B_TRANSLATE("Could not create a new file"),
				B_WARNING_ALERT);
		LogError("Invalid destination directory [%s]", dest.name);
	} else {
		ProjectItem* item = nullptr;
		if (message->FindPointer("sender", (void**)&item) == B_OK)
			GetProjectBrowser()->SelectNewItemAndScrollDelayed(item, refNew);
	}
}


void
GenioWindow::_TemplateNewFolder(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("sender_ref", &ref) != B_OK) {
		LogError("Can't find sender_ref in message!");
		return;
	}
	entry_ref refNew;
	status_t status = TemplateManager::CreateNewFolder(&ref, &refNew);
	if (status != B_OK) {
		BString error = B_TRANSLATE("Error creating folder");
		error << "\n" << strerror(status);
		OKAlert(B_TRANSLATE("New folder"), error, B_WARNING_ALERT);
		LogError("Invalid destination directory [%s]", ref.name);
	} else {
		ProjectItem* item = nullptr;
		if (message->FindPointer("sender", (void**)&item) == B_OK)
			GetProjectBrowser()->SelectNewItemAndScrollDelayed(item, refNew);
	}
}


void
GenioWindow::_TemplateNewProject(BMessage* message)
{
	LogTrace("new_folder_template");
	entry_ref template_ref;
	if (message->FindRef("refs", &template_ref) == B_OK) {
		BMessage *msg = new BMessage(MSG_CREATE_NEW_PROJECT);
		msg->AddRef("template_ref", &template_ref);
		fCreateNewProjectPanel->SetMessage(msg);
		fCreateNewProjectPanel->Show();
	}
}


void
GenioWindow::_ShowDocumentation()
{
	BStringList paths;
	BPathFinder pathFinder;
	status_t error = pathFinder.FindPaths(B_FIND_PATH_DOCUMENTATION_DIRECTORY,
		"packages/genio", paths);

	// Also add documentation path in the Genio directory
	// for when it's run directly from the repository
	BPath localDocsPath;
	if (GetGenioDirectory(localDocsPath)) {
		if (localDocsPath.GetParent(&localDocsPath) == B_OK) {
			localDocsPath.Append("documentation");
			paths.Add(localDocsPath.Path());
		}
	}
	for (int32 i = 0; i < paths.CountStrings(); ++i) {
		BPath path;
		if (error == B_OK && path.SetTo(paths.StringAt(i)) == B_OK
			&& path.Append("ReadMe.html") == B_OK) {
			BEntry entry;
			entry = path.Path();
			if (!entry.Exists())
				continue;
			BMessage message(B_REFS_RECEIVED);
			message.AddString("url", path.Path());
			be_roster->Launch("text/html", &message);
		}
	}
}


// Project Folders
void
GenioWindow::_ProjectFolderClose(ProjectFolder *project)
{
	if (project == nullptr)
		return;

	std::vector<Editor*> unsavedEditor;
	fTabManager->ForEachEditor([&](Editor* editor){
		if (editor->IsModified() && editor->GetProjectFolder() == project)
			unsavedEditor.push_back(editor);

		return true;
	});

	if (!_FileRequestSaveList(unsavedEditor))
		return;

	BString closed("Project close:");
	BString name = project->Name();

	bool wasActive = false;
	// Active project closed
	if (project == GetActiveProject()) {
		wasActive = true;
		SetActiveProject(nullptr);
		closed = "Active project close:";
		_UpdateProjectActivation(false);
		// Update run command working directory tooltip too
		BString tooltip;
		tooltip << "cwd: " << (const char*)gCFG["projects_directory"];
		fRunConsoleProgramText->SetToolTip(tooltip);
	}

	fTabManager->ReverseForEachEditor([&](Editor* editor){
		if (editor->GetProjectFolder() == project) {
			editor->SetProjectFolder(NULL);
			_RemoveTab(editor);
		}
		return true;
	});

	fProjectsFolderBrowser->ProjectFolderDepopulate(project);

	// Select a new active project
	if (wasActive) {
		ProjectFolder* project = fProjectsFolderBrowser->ProjectAt(0);
		if (project != nullptr)
			_ProjectFolderActivate(project);
	}

	// Notify subscribers that the project list has changed
	// Done here so the new active project is already set and subscribers
	// know (SourceControlPanel for example)
	if (!fDisableProjectNotifications)
		SendNotices(MSG_NOTIFY_PROJECT_LIST_CHANGED);

	project->Close();
	delete project;

	// Disable "Close project" action if no project
	if (GetProjectBrowser()->CountProjects() == 0)
		ActionManager::SetEnabled(MSG_PROJECT_CLOSE, false);

	BString notification;
	notification << closed << " " << name;
	LogInfo(notification.String());
}


status_t
GenioWindow::_ProjectFolderOpen(BMessage *message)
{
	entry_ref ref;
	status_t status = message->FindRef("refs", &ref);
	if (status == B_OK)
		status = _ProjectFolderOpen(ref);
	if (status != B_OK)
		OKAlert("Open project folder", B_TRANSLATE("Invalid project folder"), B_STOP_ALERT);

	return status;
}


status_t
GenioWindow::_ProjectFolderOpen(const entry_ref& ref, bool activate)
{
	// Preliminary checks

	BEntry dirEntry(&ref, true);
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

	BPath newProjectPath;
	status_t status = dirEntry.GetPath(&newProjectPath);
	if (status != B_OK)
		return status;

	BString newProjectPathString = newProjectPath.Path();
	newProjectPathString.Append("/");
	for (int32 index = 0; index < GetProjectBrowser()->CountProjects(); index++) {
		ProjectFolder* pProject = GetProjectBrowser()->ProjectAt(index);
		// Check if already open
		if (*pProject->EntryRef() == ref)
			return B_OK;

		BString existingProjectPath = pProject->Path();
		existingProjectPath.Append("/");
		// Check if it's a subfolder of an existing open project
		// TODO: Ideally, this wouldn't be a problem: it should be perfectly possibile
		if (newProjectPathString.StartsWith(existingProjectPath))
			return B_ERROR;
		// check if it's a parent of an existing project
		if (existingProjectPath.StartsWith(newProjectPathString))
			return B_ERROR;
	}

	// if the original entry_ref was a symlink we need to resolve the path and pass it
	// ProjectFolder
	entry_ref resolved_ref;
	if (dirEntry.GetRef(&resolved_ref) == B_ERROR)
		return B_ERROR;

	// Now open the project for real
	BMessenger msgr(this);

	// TODO: This shows a modal window
	new ProjectOpenerWindow(&resolved_ref, msgr, activate);

	return B_OK;
}


void
GenioWindow::_ProjectFolderOpenInitiated(ProjectFolder* project,
	const entry_ref& ref, bool activate)
{
	// TODO:
}


void
GenioWindow::_ProjectFolderOpenCompleted(ProjectFolder* project,
	const entry_ref& ref, bool activate)
{
	// ensure it's selected:
	GetProjectBrowser()->SelectProjectAndScroll(project);

	ActionManager::SetEnabled(MSG_PROJECT_CLOSE, true);

	BString opened("Project open: ");
	if (GetProjectBrowser()->CountProjects() == 1 || activate == true) {
		_ProjectFolderActivate(project);
		opened = "Active project open: ";
	}

	// Notify subscribers that project list has changed
	// Done here so the active project is already set
	if (!fDisableProjectNotifications)
		SendNotices(MSG_NOTIFY_PROJECT_LIST_CHANGED);

	BString projectPath = project->Path();
	BString notification;
	notification << opened << project->Name() << " at " << projectPath;
	LogInfo(notification.String());

	for (int32 i = 0; i < fTabManager->CountTabs(); i++) {
		Editor* editor = fTabManager->EditorAt(i);
		_TryAssociateEditorWithProject(editor, project);
	}

	// TODO: Move this elsewhere!
	BString taskName;
	taskName << "Detect " << project->Name() << " build system";
	Task<status_t> task
	(
		taskName,
		BMessenger(this),
		std::bind
		(
			&ProjectFolder::GuessBuildCommand,
			project
		)
	);

	task.Run();

	// final touch, let's be sure the folder is added to the recent files.
	be_roster->AddToRecentFolders(&ref, GenioNames::kApplicationSignature);
}


void
GenioWindow::_ProjectFolderOpenAborted(ProjectFolder* project,
	const entry_ref& ref, bool activate)
{
	BString notification;
	notification << "Project open fail: " << project->Name();
	LogInfo(notification.String());

	delete project;
}


status_t
GenioWindow::_OpenTerminalWorkingDirectory(const entry_ref* ref)
{
	// TODO: return value is ignored: make it void ?
	if (ref == nullptr)
		return B_BAD_VALUE;

	BEntry itemEntry(ref);
	if (!itemEntry.IsDirectory())
		itemEntry.GetParent(&itemEntry);

	BPath itemPath(&itemEntry);
	const char* argv[] = {
		"-w",
		itemPath.Path(),
		nullptr
	};
	status_t status = be_roster->Launch("application/x-vnd.Haiku-Terminal", 2, argv);
	BString notification;
	if (status != B_OK) {
		notification <<
			"An error occurred while opening Terminal and setting working directory to: ";
		notification << itemPath.Path() << ": " << ::strerror(status);
		LogError(notification.String());
	} else {
		notification <<
			"Terminal successfully opened with working directory: ";
		notification << itemPath.Path();
		LogTrace(notification.String());
	}

	return status;
}


status_t
GenioWindow::_ShowItemInTracker(const entry_ref* ref)
{
	// TODO: return value is ignored: make it void ?
	if (ref == nullptr)
		return B_BAD_VALUE;

	BEntry itemEntry(ref);
	status_t status = itemEntry.InitCheck();
	if (status == B_OK) {
		BEntry parentDirectory;
		status = itemEntry.GetParent(&parentDirectory);
		if (status == B_OK) {
			entry_ref ref;
			status = parentDirectory.GetRef(&ref);
			if (status == B_OK) {
				node_ref nref;
				status = itemEntry.GetNodeRef(&nref);
				if (status == B_OK)
					_ShowInTracker(ref, &nref);
			}
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
GenioWindow::_ShowInTracker(const entry_ref& ref, const node_ref* nref)
{
	status_t status = B_ERROR;
	BMessenger tracker("application/x-vnd.Be-TRAK");
	if (tracker.IsValid()) {
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", &ref);

		if (nref != nullptr)
			message.AddData("nodeRefToSelect", B_RAW_TYPE, (void*)nref, sizeof(node_ref));

		status = tracker.SendMessage(&message);
	}
	return status;
}


void
GenioWindow::_Replace(BMessage* message, int32 kind)
{
	if (!_ReplaceAllow())
		return;

	const BString text(fFindTextControl->Text());
	const BString replace(fReplaceTextControl->Text());

	_AddSearchFlags(message);
	message->SetString("text", text);
	message->SetString("replace", replace);
	message->SetBool("wrap", fFindWrapCheck->Value());
	message->SetInt32("kind", kind);
	_ForwardToSelectedEditor(message);
	_UpdateFindMenuItems(text);
	_UpdateReplaceMenuItems(replace);
}


bool
GenioWindow::_ReplaceAllow() const
{
	const BString selection(fFindTextControl->Text());
	const BString replacement(fReplaceTextControl->Text());
	if (selection.Length() < 1
//			|| replacement.Length() < 1
			|| selection == replacement)
		return false;

	return true;
}


void
GenioWindow::_ReplaceGroupShow(bool show)
{
	bool findGroupOpen = !fFindGroup->IsHidden();
	if (!findGroupOpen)
		_FindGroupShow(true);

	_ShowView(fReplaceGroup, show, MSG_REPLACE_GROUP_TOGGLED);

	if (!show) {
		fReplaceTextControl->TextView()->Clear();
		// If find group was not open focus and selection go there
		if (!findGroupOpen)
			_GetFocusAndSelection(fFindTextControl);
		else
			_GetFocusAndSelection(fReplaceTextControl);
	} else {
		// Replace group was opened, get focus and selection
		_GetFocusAndSelection(fReplaceTextControl);
	}
}


status_t
GenioWindow::_RunInConsole(const BString& command)
{
	// If no active project go to projects directory
	ProjectFolder* activeProject = GetActiveProject();
	if (activeProject == nullptr)
		chdir(gCFG["projects_directory"]);
	else
		chdir(activeProject->Path());

	_ShowOutputTab(kTabOutputLog);

	_UpdateRecentCommands(command);

	BMessage message;
	message.AddString("cmd", command);
	message.AddString("cmd_type", command);
	message.AddString("project_name", activeProject ? activeProject->Name() : "");

	return fMTermView->RunCommand(&message);
}


void
GenioWindow::_RunTarget()
{
	// Should not happen
	if (GetActiveProject() == nullptr)
		return;

	chdir(GetActiveProject()->Path());

	// If there's no app just return
	BEntry entry(GetActiveProject()->GetTarget());
	if (GetActiveProject()->GetTarget().IsEmpty() || !entry.Exists()) {
		LogInfoF("Target for project [%s] doesn't exist.", GetActiveProject()->Name().String());

		BString message;
		message << "Invalid run command!\n"
				   "Please configure the project to provide\n"
				   "a valid run configuration.";
		_AlertInvalidBuildConfig(message);
		return;
	}

	// Check if run args present
	BString args = GetActiveProject()->GetExecuteArgs();

	// Differentiate terminal projects from window ones
	if (GetActiveProject()->RunInTerminal()) {
		// Don't do that in graphical mode
		_UpdateProjectActivation(false);

		_ShowOutputTab(kTabOutputLog);

		BString command;
		command << GetActiveProject()->GetTarget();
		if (!args.IsEmpty())
			command << " " << args;
		// TODO: Go to appropriate directory
		// chdir(...);

		const BString projectName = GetActiveProject()->Name();
		BString claim("Run ");
		claim << projectName;
		claim << " (";
		claim << (GetActiveProject()->GetBuildMode() == BuildMode::ReleaseMode ? B_TRANSLATE("Release") : B_TRANSLATE("Debug"));
		claim << ")";

		GMessage message = {{"cmd", command},
							{"cmd_type", "build"},
							{"project_name", projectName},
							{"banner_claim", claim }};

		fMTermView->MakeFocus(true);
		fMTermView->RunCommand(&message);
	} else {
		argv_split parser(GetActiveProject()->GetTarget().String());
		parser.parse(GetActiveProject()->GetExecuteArgs().String());

		entry_ref ref;
		entry.SetTo(GetActiveProject()->GetTarget());
		entry.GetRef(&ref);
		be_roster->Launch(&ref, parser.getArguments().size() , parser.argv());
	}
}


void
GenioWindow::_ShowOutputTab(tab_id id)
{
	fPanelTabManager->ShowTab(id);
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


void
GenioWindow::_UpdateRecentCommands(const BString& text)
{
	int32 count = fRunMenuField->Menu()->CountItems();
	// Add item if not already present
	if (fRunMenuField->Menu()->FindItem(text) == nullptr) {
		BMessage*	command = new BMessage(MSG_RUN_CONSOLE_PROGRAM);
		command->AddString("command", text);
		BMenuItem* item = new BMenuItem(text, command);
		fRunMenuField->Menu()->AddItem(item, 0);
	}
	//remove last one.
	if (count == kFindReplaceMenuItems)
		fRunMenuField->Menu()->RemoveItem(count);
}


status_t
GenioWindow::_UpdateLabel(Editor* editor, bool isModified)
{
	// TODO: Would be nice to move this to GTabEditor 
	if (editor != nullptr) {
		if (isModified) {
			// Add '*' to file name
			BString label(fTabManager->TabLabel(editor));
			label.Append("*");
			fTabManager->SetTabLabel(editor, label.String());
		} else {
			// Remove '*' from file name
			BString label(fTabManager->TabLabel(editor));
			label.RemoveLast("*");
			fTabManager->SetTabLabel(editor, label.String());
		}
		return B_OK;
	}

	return B_ERROR;
}


void
GenioWindow::_UpdateProjectActivation(bool active)
{
	// TODO: Refactor here and _ProjectFolderActivate
	ActionManager::SetEnabled(MSG_CLEAN_PROJECT, active);
	fBuildModeItem->SetEnabled(active);
	fMakeCatkeysItem->SetEnabled(active);
	fMakeBindcatalogsItem->SetEnabled(active);
	ActionManager::SetEnabled(MSG_BUILD_PROJECT, active);
	fFileNewMenuItem->SetEnabled(true); // This menu should be always active!

	if (active) {
		// Is this a git project?
		try {
			if (GetActiveProject()->GetRepository()->IsInitialized())
				fGitMenu->SetEnabled(true);
			else
				fGitMenu->SetEnabled(false);
		} catch (const GitException &ex) {
		}

		// Build mode
		bool releaseMode = (GetActiveProject()->GetBuildMode() == BuildMode::ReleaseMode);

		fDebugModeItem->SetMarked(!releaseMode);
		fReleaseModeItem->SetMarked(releaseMode);

		ActionManager::SetEnabled(MSG_PROJECT_SETTINGS, true);

		ActionManager::SetEnabled(MSG_RUN_TARGET, true);
		ActionManager::SetEnabled(MSG_DEBUG_PROJECT, !releaseMode);

	} else { // here project is inactive
		fGitMenu->SetEnabled(false);
		ActionManager::SetEnabled(MSG_RUN_TARGET, false);
		ActionManager::SetEnabled(MSG_DEBUG_PROJECT, false);
		ActionManager::SetEnabled(MSG_PROJECT_SETTINGS, false);
		fFileNewMenuItem->SetViewMode(TemplatesMenu::ViewMode::SHOW_ALL_VIEW_MODE);
	}
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

	fLineEndingCRLF->SetMarked(!editor->IsReadOnly() && editor->EndOfLine() == SC_EOL_CRLF);
	fLineEndingLF->SetMarked(!editor->IsReadOnly() && editor->EndOfLine() == SC_EOL_LF);
	fLineEndingCR->SetMarked(!editor->IsReadOnly() && editor->EndOfLine() == SC_EOL_CR);
	fLineEndingsMenu->SetEnabled(!editor->IsReadOnly());

	ActionManager::SetEnabled(B_UNDO, editor->CanUndo());
	ActionManager::SetEnabled(B_REDO, editor->CanRedo());
	ActionManager::SetPressed(MSG_TEXT_OVERWRITE, editor->IsOverwrite());

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
		ActionManager::SetEnabled(MSG_FILE_CLOSE_OTHER, false);
		ActionManager::SetEnabled(MSG_FILE_CLOSE_ALL, false);

		ActionManager::SetEnabled(MSG_BUFFER_LOCK, false);
		fToolBar->SetActionEnabled(MSG_FILE_MENU_SHOW, false);

		ActionManager::SetEnabled(MSG_FILE_NEXT_SELECTED, false);
		ActionManager::SetEnabled(MSG_FILE_PREVIOUS_SELECTED, false);

		ActionManager::SetEnabled(B_SELECT_ALL, false);

		ActionManager::SetEnabled(MSG_TEXT_OVERWRITE, false);
		ActionManager::SetPressed(MSG_TEXT_OVERWRITE, false);
		ActionManager::SetEnabled(MSG_WHITE_SPACES_TOGGLE, false);
		ActionManager::SetEnabled(MSG_LINE_ENDINGS_TOGGLE, false);
		ActionManager::SetEnabled(MSG_TOGGLE_SPACES_ENDINGS, false);
		ActionManager::SetEnabled(MSG_WRAP_LINES, false);

		ActionManager::SetEnabled(MSG_DUPLICATE_LINE, false);
		ActionManager::SetEnabled(MSG_DELETE_LINES, false);
		ActionManager::SetEnabled(MSG_COMMENT_SELECTED_LINES, false);
		ActionManager::SetEnabled(MSG_FILE_TRIM_TRAILING_SPACE, false);

		ActionManager::SetEnabled(MSG_AUTOCOMPLETION, false);
		ActionManager::SetEnabled(MSG_FORMAT, false);
		ActionManager::SetEnabled(MSG_GOTODEFINITION, false);
		ActionManager::SetEnabled(MSG_GOTODECLARATION, false);
		ActionManager::SetEnabled(MSG_GOTOIMPLEMENTATION, false);
		ActionManager::SetEnabled(MSG_FIND_IN_BROWSER, false);
		ActionManager::SetEnabled(MSG_SWITCHSOURCE, false);

		fLineEndingCRLF->SetMarked(false);
		fLineEndingLF->SetMarked(false);
		fLineEndingCR->SetMarked(false);
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

		GMessage symbolNotice = {{ "what", MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED},
								 { "symbols", {{"status", Editor::STATUS_UNKNOWN}}}};
		SendNotices(MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED, &symbolNotice);
		return;
	}

	// ToolBar Items

	ActionManager::SetEnabled(MSG_FILE_FOLD_TOGGLE, editor->IsFoldingAvailable());
	ActionManager::SetEnabled(B_UNDO, editor->CanUndo());
	ActionManager::SetEnabled(B_REDO, editor->CanRedo());
	ActionManager::SetEnabled(MSG_FILE_SAVE, editor->IsModified());
	ActionManager::SetEnabled(MSG_FILE_CLOSE, true);
	ActionManager::SetEnabled(MSG_FILE_CLOSE_OTHER, fTabManager->CountTabs()>1);
	ActionManager::SetEnabled(MSG_IMPORT_RESOURCE, editor->FilePath().IEndsWith(".rdef"));

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

	ActionManager::SetEnabled(B_SELECT_ALL, true);

	ActionManager::SetEnabled(MSG_TEXT_OVERWRITE, true);
	ActionManager::SetPressed(MSG_TEXT_OVERWRITE, editor->IsOverwrite());

	ActionManager::SetEnabled(MSG_WHITE_SPACES_TOGGLE, true);
	ActionManager::SetEnabled(MSG_LINE_ENDINGS_TOGGLE, true);

	ActionManager::SetEnabled(MSG_TOGGLE_SPACES_ENDINGS, true);
	ActionManager::SetEnabled(MSG_WRAP_LINES, true);

	fLineEndingCRLF->SetMarked(!editor->IsReadOnly() && editor->EndOfLine() == SC_EOL_CRLF);
	fLineEndingLF->SetMarked(!editor->IsReadOnly() && editor->EndOfLine() == SC_EOL_LF);
	fLineEndingCR->SetMarked(!editor->IsReadOnly() && editor->EndOfLine() == SC_EOL_CR);
	fLineEndingsMenu->SetEnabled(!editor->IsReadOnly());
	fLanguageMenu->SetEnabled(true);
	//Setting the right message type:
	std::string languageName = Languages::GetMenuItemName(editor->FileType());
	for (int32 i = 0; i < fLanguageMenu->CountItems(); i++) {
		if (languageName.compare(fLanguageMenu->ItemAt(i)->Label()) == 0)  {
			fLanguageMenu->ItemAt(i)->SetMarked(true);
		}
	}

	ActionManager::SetEnabled(MSG_DUPLICATE_LINE, !editor->IsReadOnly());
	ActionManager::SetEnabled(MSG_DELETE_LINES, !editor->IsReadOnly());
	ActionManager::SetEnabled(MSG_COMMENT_SELECTED_LINES, !editor->IsReadOnly());
	ActionManager::SetEnabled(MSG_FILE_TRIM_TRAILING_SPACE, !editor->IsReadOnly());

	ActionManager::SetEnabled(MSG_AUTOCOMPLETION, !editor->IsReadOnly() && editor->HasLSPCapability(kLCapCompletion));
	ActionManager::SetEnabled(MSG_FORMAT, !editor->IsReadOnly() && editor->HasLSPCapability(kLCapDocFormatting));
	ActionManager::SetEnabled(MSG_GOTODEFINITION, editor->HasLSPCapability(kLCapDefinition));
	ActionManager::SetEnabled(MSG_GOTODECLARATION, editor->HasLSPCapability(kLCapDeclaration));
	ActionManager::SetEnabled(MSG_GOTOIMPLEMENTATION, editor->HasLSPCapability(kLCapImplementation));
	ActionManager::SetEnabled(MSG_RENAME, editor->HasLSPCapability(kLCapRename));
	ActionManager::SetEnabled(MSG_FIND_IN_BROWSER, editor->GetProjectFolder() != nullptr);
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

	fProblemsPanel->UpdateProblems(editor);

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
	BString context = message->GetString("context", "");
	if (context.IsEmpty() || context.Compare("reset_to_defaults_end") == 0) {
		for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
			Editor* editor = fTabManager->EditorAt(index);
			editor->LoadEditorConfig();
			editor->ApplySettings();
		}
	}

	if (key.StartsWith("find_")) {
		fFindWrapCheck->SetValue(gCFG["find_wrap"] ? B_CONTROL_ON : B_CONTROL_OFF);
		fFindWholeWordCheck->SetValue(gCFG["find_whole_word"] ? B_CONTROL_ON : B_CONTROL_OFF);
		fFindCaseSensitiveCheck->SetValue(gCFG["find_match_case"] ? B_CONTROL_ON : B_CONTROL_OFF);
	} else if (key.Compare("wrap_lines") == 0) {
		ActionManager::SetPressed(MSG_WRAP_LINES, gCFG["wrap_lines"]);
	} else if (key.Compare("wrap_console") == 0) {
//		fBuildLogView->SetWordWrap(gCFG["wrap_console"]);
	} else if (key.Compare("show_white_space") == 0) {
		ActionManager::SetPressed(MSG_WHITE_SPACES_TOGGLE, gCFG["show_white_space"]);
		bool same = ((bool)gCFG["show_white_space"] && (bool)gCFG["show_line_endings"]);
		ActionManager::SetPressed(MSG_TOGGLE_SPACES_ENDINGS, same);
	} else if (key.Compare("show_line_endings") == 0) {
		ActionManager::SetPressed(MSG_LINE_ENDINGS_TOGGLE, gCFG["show_line_endings"]);
		bool same = ((bool)gCFG["show_white_space"] && (bool)gCFG["show_line_endings"]);
		ActionManager::SetPressed(MSG_TOGGLE_SPACES_ENDINGS, same);
	} else if (key.Compare("show_projects") == 0) {
		_ShowPanelTabView(kTabViewLeft,   gCFG["show_projects"], MSG_SHOW_HIDE_LEFT_PANE);
	} else if (key.Compare("show_outline") == 0) {
		_ShowPanelTabView(kTabViewRight,  gCFG["show_outline"], MSG_SHOW_HIDE_RIGHT_PANE);
	} else if (key.Compare("show_output") == 0) {
		_ShowPanelTabView(kTabViewBottom, gCFG["show_output"],	MSG_SHOW_HIDE_BOTTOM_PANE);
	} else if (key.Compare("show_toolbar") == 0) {
		_ShowView(fToolBar, bool(gCFG["show_toolbar"]), MSG_TOGGLE_TOOLBAR);
	} else if (key.Compare("show_statusbar") == 0) {
		_ShowView(fStatusView, bool(gCFG["show_statusbar"]), MSG_TOGGLE_STATUSBAR);
	} else if (key.Compare("use_small_icons") == 0) {
		float iconSize = 0;
		if (message->GetBool("value", false))
			iconSize = be_control_look->ComposeIconSize(kDefaultIconSizeSmall).Width();
		else
			iconSize = be_control_look->ComposeIconSize(kDefaultIconSize).Width();

		fToolBar->ChangeIconSize(iconSize);
		fFindGroup->ChangeIconSize(iconSize);
		fReplaceGroup->ChangeIconSize(iconSize);
		fRunGroup->ChangeIconSize(iconSize);
	}

	Editor* selected = fTabManager->SelectedEditor();
	_UpdateWindowTitle(selected ? selected->FilePath().String() : nullptr);
}


void
GenioWindow::_HandleProjectConfigurationChanged(BMessage* message)
{
	// TODO: This could go into ProjectBrowser in part or entirely
	const ProjectFolder* project
		= reinterpret_cast<const ProjectFolder*>(message->GetPointer("project_folder", nullptr));
	if (project == nullptr) {
		LogError("GenioWindow: Update project configuration message without a project folder pointer!");
		return;
	}
	BString key(message->GetString("key", ""));
	if (key.IsEmpty())
		return;

	if (project == GetActiveProject() || GetActiveProject() == nullptr) {
		// Update debug/release
		_UpdateProjectActivation(GetActiveProject() != nullptr);
	}

	// TODO: refactor
	if (key == "color") {
		for (int32 index = 0; index < fTabManager->CountTabs(); index++) {
			Editor* editor = fTabManager->EditorAt(index);
			ProjectFolder* project = editor->GetProjectFolder();
			if (project != nullptr) {
				fTabManager->SetTabColor(editor, project->Color());
			}
		}
	}
}


void
GenioWindow::UpdateMenu(const void* sender, const entry_ref* ref)
{
	fFileNewMenuItem->SetSender(sender, ref);
}


void
GenioWindow::_UpdateWindowTitle(const char* filePath)
{
	BString title;

#ifdef GDEBUG
	if (!fTitlePrefix.IsEmpty())
		title << fTitlePrefix << " ";
#endif
	title << GenioNames::kApplicationName;
	// File full path in window title
	if (gCFG["fullpath_title"] && filePath != nullptr)
		title << ": " << filePath;
	SetTitle(title.String());
}
