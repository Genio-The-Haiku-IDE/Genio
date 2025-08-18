/*
 * Copyright 2017..2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <String.h>
#include <Window.h>

#include <vector>

#include "GMessage.h"
#include "PanelTabManager.h"


enum scree_mode {
	kDefault = 0,
	kFullscreen,
	kFocus
};


class ActionManager;
class BCheckBox;
class BFilePanel;
class BGroupLayout;
class BMenu;
class BMenuField;
class BTabView;
class BTextControl;
class ConsoleIOTab;
class Editor;
class GoToLineWindow;
class ProblemsPanel;
class ProjectFolder;
class ProjectBrowser;
class SearchResultTab;
class SourceControlPanel;
class TemplatesMenu;
class ToolBar;
class MTermView;
class PanelTabManager;
class EditorTabView;
class TerminalTab;
class ConsoleIOTabView;

class GenioWindow : public BWindow {
public:
								GenioWindow(BRect frame);
	virtual						~GenioWindow();

	virtual void				MessageReceived(BMessage* message);
	virtual void				MenusBeginning();
	virtual void				MenusEnded();
	virtual bool				QuitRequested();
	virtual void				Show();

	void						UpdateMenu(const void* sender, const entry_ref* ref);
	ProjectFolder*				GetActiveProject() const;
	void						SetActiveProject(ProjectFolder *project);
	ProjectBrowser*		GetProjectBrowser() const;

	EditorTabView*			TabManager() const;

private:
			void				_PrepareWorkspace();

			Editor*				_AddEditorTab(entry_ref* ref, BMessage* addInfo);
			status_t			_RemoveTab(Editor* editor);

			status_t			_BuildProject();
			status_t			_CleanProject();
			status_t			_DebugProject();
			status_t			_DoBuildOrCleanProject(const BString& cmd);

			bool				_FileRequestClose(Editor* editor);
			void				_FileCloseAll();
			bool				_FileRequestSaveList(std::vector<Editor*>& unsavedEditor);
			bool				_FileRequestSaveAllModified();

			status_t			_FileOpen(BMessage* msg);
			status_t			_FileOpenAtStartup(BMessage* msg);
			status_t			_FileOpenWithPosition(entry_ref* ref, bool openWithPreferred,  int32 be_line, int32 lsp_char);
			status_t            _FileOpenWithPreferredApp(const entry_ref* ref);
			bool				_FileIsSupported(const entry_ref* ref);

			status_t			_FileSave(Editor* editor);
			void				_FileSaveAll(ProjectFolder* onlyThisProject = NULL);
			status_t			_FileSaveAs(Editor* , BMessage* message);
			int32				_FilesNeedSave();

			void				_PreFileLoad(Editor* editor);
			void				_PostFileLoad(Editor* editor);
			void				_PreFileSave(Editor* editor);
			void				_PostFileSave(Editor* editor);

			void				_FindGroupShow(bool show);
			void				_FindMarkAll(BMessage* message);
			void				_FindNext(BMessage* message, bool backwards);
			void				_FindInFiles();
			void				_AddSearchFlags(BMessage* msg);

			status_t			_SelectEditorToPosition(Editor* editor, int32 be_line, int32 lsp_char);
			void				_ApplyEditsToSelectedEditor(BMessage* msg);
			void 				_HandleEditorZoom(int32 changeValue);

			void				_GetFocusAndSelection(BTextControl* control) const;
			status_t			_Git(const BString& git_command);
			void				_HandleExternalMoveModification(entry_ref* oldRef, entry_ref* newRef);
			void				_HandleExternalRemoveModification(Editor* editor);
			void				_HandleExternalStatModification(Editor* editor);
			void				_HandleNodeMonitorMsg(BMessage* msg);
			void				_CheckEntryRemoved(BMessage* msg);
			void				_InitCentralSplit();
			void				_InitCommandRunToolbar();
			void				_InitMenu();
			void				_InitTabViews();
			void				_InitToolbar();
			void				_InitWindow();

			void				_MakeBindcatalogs();
			void				_MakeCatkeys();

			void				_ProjectFileDelete(BMessage* message);
			void				_ProjectRenameFile(BMessage* message);

			void				_TemplateNewFolder(BMessage* message);
			void				_TemplateNewProject(BMessage* message);
			void				_TemplateNewFile(BMessage* message);

			void				_ShowDocumentation();

			// Project Folders (TODO: Rename to Project*, without "Folder")
			void				_ProjectFolderClose(ProjectFolder *project);
			status_t			_ProjectFolderOpen(BMessage *message);
			status_t			_ProjectFolderOpen(const entry_ref& ref, bool activate = false);

			void				_ProjectFolderOpenInitiated(ProjectFolder* project,
															const entry_ref& ref, bool activate);
			void				_ProjectFolderOpenCompleted(ProjectFolder* project,
															const entry_ref& ref, bool activate);
			void				_ProjectFolderOpenAborted(ProjectFolder* project,
															const entry_ref& ref, bool activate);

			void				_ProjectFolderActivate(ProjectFolder* project);
			void				_TryAssociateEditorWithProject(Editor* editor, ProjectFolder* project);

			status_t			_ShowItemInTracker(const entry_ref*);
			status_t			_ShowInTracker(const entry_ref& ref, const node_ref* nref = NULL);
			status_t			_OpenTerminalWorkingDirectory(const entry_ref* ref);

			void				_Replace(BMessage* message, int32 kind);
			bool				_ReplaceAllow() const;
			void				_ReplaceGroupShow(bool show);
			status_t			_RunInConsole(const BString& command);
			void				_RunTarget();

			void				_ShowOutputTab(tab_id id);

			void				_UpdateFindMenuItems(const BString& text);
			void				_UpdateRecentCommands(const BString& text);
			status_t			_UpdateLabel(Editor* editor, bool isModified);
			void				_UpdateProjectMenuItemsState(bool enable);
			void				_UpdateReplaceMenuItems(const BString& text);
			void				_UpdateSavepointChange(Editor*, const BString& caller = "");
			void				_UpdateTabChange(Editor*, const BString& caller = "");
			void				_InitActions();
			void				_ShowView(BView*, bool show, int32 msgWhat = -1);
			void				_ShowPanelTabView(const char* name, bool show, int32 msgWhat = -1);
			status_t			_AlertInvalidBuildConfig(BString text);
			void				_CloseMultipleTabs(std::vector<Editor*>& editors);
			void				_HandleConfigurationChanged(BMessage* msg);
			void				_HandleProjectConfigurationChanged(BMessage* message);
			BMenu*				_CreateLanguagesMenu();
			void				_ToogleScreenMode(int32 action);
			void				_ForwardToSelectedEditor(BMessage* msg);
			void				_UpdateWindowTitle(const char* filePath, const char* currentBranch);

private:
			BMenuBar*			fMenuBar;
			TemplatesMenu*		fFileNewMenuItem;

			BMenu*				fLineEndingsMenu;
			BMenuItem*			fLineEndingCRLF;
			BMenuItem*			fLineEndingLF;
			BMenuItem*			fLineEndingCR;
			BMenu*				fLanguageMenu;
			BMenu*				fBookmarksMenu;
			BMenuItem*			fBookmarkToggleItem;
			BMenuItem*			fBookmarkClearAllItem;
			BMenuItem*			fBookmarkGoToNextItem;
			BMenuItem*			fBookmarkGoToPreviousItem;

			BMenu*				fBuildModeItem;
			BMenuItem*			fReleaseModeItem;
			BMenuItem*			fDebugModeItem;
			BMenuItem*			fMakeCatkeysItem;
			BMenuItem*			fMakeBindcatalogsItem;

			BMenu*				fSetActiveProjectMenuItem;

			BMenu*				fGitMenu;
			BMenuItem*			fGitBranchItem;
			BMenuItem*			fGitLogItem;
			BMenuItem*			fGitLogOnelineItem;
			BMenuItem*			fGitPullItem;
			BMenuItem*			fGitPullRebaseItem;
			BMenuItem*			fGitShowConfigItem;
			BMenuItem*			fGitStatusItem;
			BMenuItem*			fGitStatusShortItem;
			BMenuItem*			fGitTagItem;

			ToolBar*			fToolBar;

			BGroupLayout*		fRootLayout;
			BGroupLayout*		fEditorTabsGroup;


			ProjectBrowser*		fProjectsFolderBrowser;
			BScrollView*		fProjectsFolderScroll;

			SourceControlPanel*	fSourceControlPanel;
			BScrollView*		fSourceControlPanelScroll;

			ProjectFolder		*fActiveProject;

			// Editor group
			EditorTabView*		fTabManager;

			ToolBar*			fFindGroup;
			ToolBar*			fReplaceGroup;
			BView*				fStatusView;
			BMenuField*			fFindMenuField;
			BMenuField*			fReplaceMenuField;
			BTextControl*		fFindTextControl;
			BTextControl*		fReplaceTextControl;
			BCheckBox*			fFindCaseSensitiveCheck;
			BCheckBox*			fFindWholeWordCheck;
			BCheckBox*			fFindWrapCheck;
			ToolBar*			fRunGroup;
			BTextControl*		fRunConsoleProgramText;
			BMenuField* 		fRunMenuField;
			BString				fConsoleStdinLine;

			BFilePanel*			fOpenPanel;
			BFilePanel*			fSavePanel;
			BFilePanel*			fOpenProjectPanel;
			BFilePanel*			fOpenProjectFolderPanel;

			// Bottom panels
			ProblemsPanel*		fProblemsPanel;
			ConsoleIOTabView*	fBuildLogView;
			ConsoleIOTabView*	fMTermView;
			GoToLineWindow*		fGoToLineWindow;
			SearchResultTab*	fSearchResultTab;

			scree_mode			fScreenMode;
			GMessage			fScreenModeSettings;

#ifdef GDEBUG
			BString				fTitlePrefix;
#endif
			PanelTabManager*	fPanelTabManager;
			BMenu*				fPanelsMenu;

};

extern GenioWindow *gMainWindow;
