/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <View.h>

#include <ObjectList.h>


enum {
	MSG_PROJECT_MENU_CLOSE				= 'pmcl',
	MSG_PROJECT_MENU_SET_ACTIVE			= 'pmsa',
	MSG_PROJECT_MENU_DELETE_FILE		= 'pmdf',
	MSG_PROJECT_MENU_SHOW_IN_TRACKER	= 'pmst',
	MSG_PROJECT_MENU_OPEN_TERMINAL		= 'pmot',
	MSG_PROJECT_MENU_OPEN_FILE			= 'pmof',
	MSG_PROJECT_MENU_NEW_FILE			= 'pmnf',
	MSG_PROJECT_MENU_RENAME_FILE		= 'pmrf',
	MSG_PROJECT_MENU_DO_RENAME_FILE		= 'pmdr',

	MSG_BROWSER_SELECT_ITEM				= 'sele'
};

class ProjectOutlineListView;
class ProjectFolder;
class ProjectItem;
class GenioWatchingFilter;

class ProjectBrowser : public BView {
public:
					 ProjectBrowser();
	virtual 		~ProjectBrowser();

	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	MessageReceived(BMessage* message);

	ProjectItem*	GetSelectedProjectItem() const;
	const entry_ref* GetSelectedProjectFileRef() const;

	ProjectItem*	GetItemByRef(ProjectFolder* project, const entry_ref& ref) const;

	ProjectItem*	GetProjectItemForProject(ProjectFolder*) const;

	ProjectFolder*	GetProjectFromItem(ProjectItem*) const;
	ProjectFolder*	GetProjectFromSelectedItem() const;

	int32			CountProjects() const;
	ProjectFolder*	ProjectAt(int32 index) const;
	ProjectFolder*	ProjectByPath(const BString& fullPath) const;

	const 			BObjectList<ProjectFolder>* GetProjectList() const;

	void			SelectProjectAndScroll(ProjectFolder*);

	void			SelectNewItemAndScrollDelayed(ProjectItem* parent, const entry_ref ref); //ugly name..
	void			SelectItemByRef(ProjectFolder* project, const entry_ref& ref);

	void			ProjectFolderPopulate(ProjectFolder* project);
	void			ProjectFolderDepopulate(ProjectFolder* project);

	void			ExpandActiveProjects();

	void			InitRename(ProjectItem *item);
private:

	ProjectItem*	GetProjectItemByPath(const BString& path) const;

	ProjectItem*	_CreatePath(BPath pathToCreate);

	ProjectItem*	_ProjectFolderScan(ProjectItem* item, const entry_ref* ref, ProjectFolder *projectFolder = NULL);

	void			_ShowProjectItemPopupMenu(BPoint where);

	void			_UpdateNode(BMessage *message);

	status_t		_RenameCurrentSelectedFile(const BString& newName);

	ProjectItem*	_CreateNewProjectItem(ProjectItem* parentItem, BPath path);

private:
	ProjectOutlineListView*	fOutlineListView;
	bool					fIsBuilding;
	GenioWatchingFilter*	fGenioWatchingFilter;

	//TODO: remove this and use a std::vector<std::pair or similar.
	BObjectList<ProjectFolder>	fProjectList;
	BObjectList<ProjectItem>	fProjectProjectItemList;
};
