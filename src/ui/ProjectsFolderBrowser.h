/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ProjectsFolderBrowser_H
#define ProjectsFolderBrowser_H


#include <OutlineListView.h>
#include <ObjectList.h>

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

	virtual	void	MouseUp(BPoint where);
	virtual void	MouseDown(BPoint where);
	virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	MessageReceived(BMessage* message);

	ProjectItem*	GetSelectedProjectItem() const;
	ProjectItem*	GetProjectItemForProject(ProjectFolder*);

	ProjectFolder*	GetProjectFromItem(ProjectItem*) const;
	ProjectFolder*	GetProjectFromSelectedItem() const;

	const entry_ref* GetSelectedProjectFileRef() const;

	void			ProjectFolderPopulate(ProjectFolder* project);
	void			ProjectFolderDepopulate(ProjectFolder* project);

	virtual void	SelectionChanged();

	void			InitRename(ProjectItem *item);

	int32			CountProjects() const;
	ProjectFolder*	ProjectAt(int32 index) const;
	ProjectFolder*	ProjectByPath(const BString& fullPath) const;

	const BObjectList<ProjectFolder>*	GetProjectList() const;

	void			SelectAndScroll(ProjectFolder*);

private:

	ProjectItem*	GetProjectItemByPath(const BString& path) const;

	ProjectItem*	_CreatePath(BPath pathToCreate);

	ProjectItem*	_ProjectFolderScan(ProjectItem* item, const entry_ref* ref, ProjectFolder *projectFolder = NULL);

	void			_ShowProjectItemPopupMenu(BPoint where);

	static	int		_CompareProjectItems(const BListItem* a, const BListItem* b);

	void			_UpdateNode(BMessage *message);

	status_t		_RenameCurrentSelectedFile(const BString& newName);

	ProjectItem*	_CreateNewProjectItem(ProjectItem* parentItem, BPath path);

private:
	TemplatesMenu*		fFileNewProjectMenuItem;

	bool				fIsBuilding = false;
	GenioWatchingFilter* fGenioWatchingFilter;

	//TODO: remove this and use a std::vector<std::pair or similar.
	BObjectList<ProjectFolder>	fProjectList;
	BObjectList<ProjectItem>	fProjectProjectItemList;
};


#endif // ProjectsFolderBrowser_H
