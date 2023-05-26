/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ProjectsFolderBrowser_H
#define ProjectsFolderBrowser_H


#include <SupportDefs.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>

#include "TemplatesMenu.h"

enum {
	MSG_PROJECT_MENU_CLOSE				= 'pmcl',
	MSG_PROJECT_MENU_SET_ACTIVE			= 'pmsa',
	MSG_PROJECT_MENU_DELETE_FILE		= 'pmdf',
	MSG_PROJECT_MENU_SHOW_IN_TRACKER	= 'pmst',
	MSG_PROJECT_MENU_OPEN_TERMINAL		= 'pmot',
	MSG_PROJECT_MENU_OPEN_FILE			= 'pmof',
	MSG_PROJECT_MENU_NEW_FILE			= 'pmnf',
	MSG_PROJECT_MENU_RENAME_FILE		= 'pmrf',
	MSG_PROJECT_MENU_DO_RENAME_FILE		= 'pmdr'
};

class ProjectFolder;
class ProjectItem;
class GenioWatchingFilter;

class ProjectsFolderBrowser : public BOutlineListView {
public:
					 ProjectsFolderBrowser();
	virtual 		~ProjectsFolderBrowser();
	
	void			MouseDown(BPoint where);
	void			AttachedToWindow();
	void			MessageReceived(BMessage* message);
	
	ProjectFolder*	GetProjectFromCurrentItem();
	
	ProjectItem*	GetCurrentProjectItem();
	
	BString const	GetCurrentProjectFileFullPath();
	
	void			SetBuildingPhase(bool building) { fIsBuilding = building;};
	
	void			ProjectFolderPopulate(ProjectFolder* project);
	void			ProjectFolderDepopulate(ProjectFolder* project);
	
	virtual void	SelectionChanged();
	
	void			InitRename(ProjectItem *item);
	
private:
	
	ProjectItem*	FindProjectItem(BString const& path);
	
	ProjectItem*	_CreatePath(BPath pathToCreate);
	
	void			_ProjectFolderScan(ProjectItem* item, BString const& path, ProjectFolder *projectFolder = NULL);

	void			_ShowProjectItemPopupMenu(BPoint where);
	ProjectFolder*	_GetProjectFromItem(ProjectItem*);
	
	static	int		_CompareProjectItems(const BListItem* a, const BListItem* b);
	
	void			_UpdateNode(BMessage *message);
	
	status_t		_RenameCurrentSelectedFile(const BString& newName);

	ProjectItem*	_CreateNewProjectItem(ProjectItem* parentItem, BPath path);
	
private:
	
	BPopUpMenu*			fProjectMenu;
	BMenuItem*			fCloseProjectMenuItem;
	BMenuItem*			fRenameFileProjectMenuItem;
	BMenuItem*			fSetActiveProjectMenuItem;
	BMenuItem*			fDeleteFileProjectMenuItem;
	BMenuItem*			fOpenFileProjectMenuItem;
	TemplatesMenu*		fFileNewProjectMenuItem;
	BMenuItem*			fShowInTrackerProjectMenuItem;
	BMenuItem*			fOpenTerminalProjectMenuItem;
	bool				fIsBuilding = false;
	GenioWatchingFilter* fGenioWatchingFilter;
	
};


#endif // ProjectsFolderBrowser_H
