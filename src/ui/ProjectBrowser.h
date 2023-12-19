/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <OutlineListView.h>

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

	virtual void	MouseDown(BPoint where);
	virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	MessageReceived(BMessage* message);

	ProjectItem*	GetProjectItem(const BString& projectName) const;
	ProjectItem*	GetProjectItemAt(const int32& index) const;
	ProjectItem*	GetProjectItemByPath(const BString& path) const;
	ProjectItem*	GetSelectedProjectItem() const;

	ProjectFolder*	GetProjectFromItem(ProjectItem*) const;
	ProjectFolder*	GetProjectFromSelectedItem() const;

	BString const	GetSelectedProjectFileFullPath() const;

	void			ProjectFolderPopulate(ProjectFolder* project);
	void			ProjectFolderDepopulate(ProjectFolder* project);

	virtual void	SelectionChanged();

	void			InitRename(ProjectItem *item);

private:
	ProjectItem*	_CreatePath(BPath pathToCreate);

	void			_ProjectFolderScan(ProjectItem* item, BString const& path, ProjectFolder *projectFolder = NULL);

	void			_ShowProjectItemPopupMenu(BPoint where);

	static	int		_CompareProjectItems(const BListItem* a, const BListItem* b);

	void			_UpdateNode(BMessage *message);

	status_t		_RenameCurrentSelectedFile(const BString& newName);

	ProjectItem*	_CreateNewProjectItem(ProjectItem* parentItem, BPath path);

private:
	TemplatesMenu*		fFileNewProjectMenuItem;

	bool				fIsBuilding = false;
	GenioWatchingFilter* fGenioWatchingFilter;

};