/*
 * Copyright 2017..2018 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GenioWINDOW_H
#define GenioWINDOW_H

#include <map>

#include <Bitmap.h>
#include <CheckBox.h>
#include <ColumnTypes.h>
#include <FilePanel.h>
#include <GroupLayout.h>
#include <MenuBar.h>
#include <ObjectList.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StatusBar.h>
#include <String.h>
#include <TabView.h>
#include <TextControl.h>
#include <Window.h>
#include "ToolBar.h"
#include "GoToLineWindow.h"
#include "ProblemsPanel.h"
#include "ConsoleIOView.h"
#include "Editor.h"
#include "ProjectFolder.h"
#include "EditorTabManager.h"
#include "ProjectsFolderBrowser.h"
#include "TemplatesMenu.h"
#include "SearchResultPanel.h"
#include <vector>

enum {
	kProjectsOutline = 0,
};


enum {
	kProblems = 0,
	kBuildLog,
	kOutputLog
};

class ActionManager;

class GenioWindow : public BWindow
{
public:
								GenioWindow(BRect frame);
	virtual						~GenioWindow();

	virtual void				DispatchMessage(BMessage* message, BHandler* handler);
	virtual void				MessageReceived(BMessage* message);
	virtual bool				QuitRequested();
	virtual void				Show();

	void						UpdateMenu();
	ProjectFolder*				GetActiveProject() { return fActiveProject; }

private:

			status_t			_AddEditorTab(entry_ref* ref, int32 index, int32 be_line, int32 sci_pos);

			void				_BuildDone(BMessage* msg);
			status_t			_BuildProject();
			status_t			_CargoNew(BString args);
			status_t			_CleanProject();


			status_t			_DebugProject();
			bool				_FileRequestClose(int32 index);
			status_t			_RemoveTab(int32 index);
			void				_FileCloseAll();
			bool				_FileRequestSaveList(std::vector<int32>& unsavedIndex);
			bool				_FileRequestSaveAllModified();
			status_t			_FileOpen(BMessage* msg);
			bool				_FileIsSupported(const entry_ref* ref);
			status_t            _FileOpenWithPreferredApp(const entry_ref* ref);
			status_t			_FileSave(int32	index);
			void				_FileSaveAll(ProjectFolder* onlyThisProject = NULL);
			status_t			_FileSaveAs(int32 selection, BMessage* message);
			int32				_FilesNeedSave();
			void				_PreFileSave(Editor* editor);
			void				_PostFileSave(Editor* editor);
			void				_FindGroupShow(bool show);
			int32				_FindMarkAll(const BString text);
			void				_FindNext(const BString& strToFind, bool backwards);
			void				_FindInFiles();

			int32				_GetEditorIndex(entry_ref* ref, bool checkExists = false);
			int32				_GetEditorIndex(node_ref* nref);
			void				_GetFocusAndSelection(BTextControl* control);
			status_t			_Git(const BString& git_command);
			void				_HandleExternalMoveModification(entry_ref* oldRef, entry_ref* newRef);
			void				_HandleExternalRemoveModification(int32 index);
			void				_HandleExternalStatModification(int32 index);
			void				_HandleNodeMonitorMsg(BMessage* msg);
			void				_InitCentralSplit();
			void				_InitMenu();
			void				_InitOutputSplit();
			void				_InitSideSplit();
			void				_InitToolbar();
			void				_InitWindow();

			void				_MakeBindcatalogs();
			void				_MakeCatkeys();

			void				_ProjectFileDelete();
			void				_ProjectRenameFile();

			status_t			_ProjectRemoveDir(const BString& dirPath);

			// Project Folders
			void				_ProjectFolderClose(ProjectFolder *project);
			void 				_ProjectFolderOpen(BMessage *message);
			void				_ProjectFolderOpen(const BString& folder, bool activate = false);
			void				_ProjectFolderActivate(ProjectFolder* project);

			status_t			_ShowCurrentItemInTracker();
			status_t			_ShowInTracker(entry_ref *ref);
			status_t			_OpenTerminalWorkingDirectory();

			int					_Replace(int what);
			bool				_ReplaceAllow();
			void				_ReplaceGroupShow(bool show);
			status_t			_RunInConsole(const BString& command);
			void				_RunTarget();
			void				_SetMakefileBuildMode();
			void				_ShowLog(int32 index);
			void				_UpdateFindMenuItems(const BString& text);
			status_t			_UpdateLabel(int32 index, bool isModified);
			void				_UpdateProjectActivation(bool active);
			void				_UpdateReplaceMenuItems(const BString& text);
			void				_UpdateSavepointChange(int32 index, const BString& caller = "");
			void				_UpdateTabChange(Editor*, const BString& caller = "");
			void				_InitActions();
			void				_ShowView(BView*, bool show, int32 msgWhat = -1);
			status_t			_AlertInvalidBuildConfig(BString text);
			void				_CloseMultipleTabs(BMessage* msg);
			void				_HandleConfigurationChanged(BMessage* msg);

private:
			BMenuBar*			fMenuBar;
			TemplatesMenu*	fFileNewMenuItem;

      BMenu*				fLineEndingsMenu;
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


			// Left panels
			BTabView*	  		fProjectsTabView;

			ProjectsFolderBrowser*	fProjectsFolderBrowser;
			BScrollView*		fProjectsFolderScroll;


			ProjectFolder		*fActiveProject;
			bool				fIsBuilding;

			BObjectList<ProjectFolder>*	fProjectFolderObjectList;

			// Editor group
			EditorTabManager*		fTabManager;


			ToolBar*			fFindGroup;
			ToolBar*			fReplaceGroup;
			BMenuField*			fFindMenuField;
			BMenuField*			fReplaceMenuField;
			BTextControl*		fFindTextControl;
			BTextControl*		fReplaceTextControl;
			BCheckBox*			fFindCaseSensitiveCheck;
			BCheckBox*			fFindWholeWordCheck;
			BCheckBox*			fFindWrapCheck;
			BGroupLayout*		fRunConsoleProgramGroup;
			BTextControl*		fRunConsoleProgramText;
			BButton*			fRunConsoleProgramButton;
			BString				fConsoleStdinLine;

			BFilePanel*			fOpenPanel;
			BFilePanel*			fSavePanel;
			BFilePanel*			fOpenProjectPanel;
			BFilePanel*			fOpenProjectFolderPanel;
			BFilePanel*			fCreateNewProjectPanel;

			// Bottom panels
			BTabView*			fOutputTabView;
			ProblemsPanel*		fProblemsPanel;
			ConsoleIOView*		fBuildLogView;
			ConsoleIOView*		fConsoleIOView;
			GoToLineWindow*		fGoToLineWindow;
			SearchResultPanel*	fSearchResultPanel;
};

#endif //GenioWINDOW_H
