/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ProjectsFolderBrowser_H
#define ProjectsFolderBrowser_H


#include <SupportDefs.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>

enum {
	MSG_PROJECT_MENU_CLOSE				= 'pmcl',
	MSG_PROJECT_MENU_SET_ACTIVE			= 'pmsa',
	MSG_PROJECT_MENU_ADD_ITEM			= 'pmai',
	MSG_PROJECT_MENU_DELETE_FILE		= 'pmdf',
	MSG_PROJECT_MENU_EXCLUDE_FILE		= 'pmef',
	MSG_PROJECT_MENU_OPEN_FILE			= 'pmof',
	MSG_PROJECT_MENU_SHOW_IN_TRACKER	= 'pmst',
	MSG_PROJECT_MENU_OPEN_TERMINAL		= 'pmot',
};

class ProjectFolder;
class ProjectItem;

class ProjectsFolderBrowser : public BOutlineListView {
public:
					 ProjectsFolderBrowser();
			virtual ~ProjectsFolderBrowser();
	
	void	MouseDown(BPoint where);
	void	AttachedToWindow();
	void	MessageReceived(BMessage* message);
	
	ProjectFolder*	GetProjectFromCurrentItem();
	
	ProjectItem*	GetCurrentProjectItem();
	
	BString const	GetCurrentProjectFileFullPath();
	
	void	SetBuildingPhase(bool building) { fIsBuilding = building;};
	
	void	ProjectFolderPopulate(ProjectFolder* project);
	void	ProjectFolderDepopulate(ProjectFolder* project);
	
private:
	
	void			_ProjectFolderScan(ProjectItem* item, BString const& path, ProjectFolder *projectFolder = NULL);

	void			_ShowProjectItemPopupMenu(BPoint where);
	ProjectFolder*	_GetProjectFromItem(ProjectItem*);
	
	static	int		_CompareProjectItems(const BListItem* a, const BListItem* b);
	
private:
	
	BPopUpMenu*			fProjectMenu;
	BMenuItem*			fCloseProjectMenuItem;
	BMenuItem*			fDeleteProjectMenuItem;
	BMenuItem*			fSetActiveProjectMenuItem;
	BMenuItem*			fAddProjectMenuItem;
	BMenuItem*			fExcludeFileProjectMenuItem;
	BMenuItem*			fDeleteFileProjectMenuItem;
	BMenuItem*			fOpenFileProjectMenuItem;
	BMenuItem*			fShowInTrackerProjectMenuItem;
	BMenuItem*			fOpenTerminalProjectMenuItem;
	bool				fIsBuilding = false;
};


#endif // ProjectsFolderBrowser_H
