/*
 * Copyright 2023, Andrea Anzani 
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

	ProjectItem*	GetItemByRef(const ProjectFolder* project, const entry_ref& ref) const;

	ProjectItem*	GetProjectItemForProject(const ProjectFolder*) const;

	ProjectFolder*	GetProjectFromItem(const ProjectItem*) const;
	ProjectFolder*	GetProjectFromSelectedItem() const;

	int32			CountProjects() const;
	ProjectFolder*	ProjectAt(int32 index) const;
	ProjectFolder*	ProjectByPath(const BString& fullPath) const;

	const 			BObjectList<ProjectFolder>* GetProjectList() const;

	void			SelectProjectAndScroll(const ProjectFolder*);

	void			SelectItemByRef(const ProjectFolder* project, const entry_ref& ref);
	void			SelectNewItemAndScrollDelayed(const ProjectItem* parent, const entry_ref ref); //ugly name..

	void			ProjectFolderPopulate(ProjectFolder* project);
	void			ProjectFolderDepopulate(ProjectFolder* project);

	void			ExpandProjectCollapseOther(const BString& projectName);

	void			InitRename(ProjectItem *item);
private:

	ProjectItem*	GetProjectItemByPath(const BString& path) const;

	ProjectItem*	_ProjectFolderScan(ProjectItem* item, const entry_ref* ref, ProjectFolder *projectFolder = NULL);

	void			_ShowProjectItemPopupMenu(BPoint where);

	ProjectItem*	_CreatePath(BPath pathToCreate);
	void			_RemovePath(BString pathToRemove);
	void			_HandleEntryMoved(BMessage* message);
	void			_UpdateNode(BMessage *message);

	status_t		_RenameCurrentSelectedFile(const BString& newName);

	ProjectItem*	_CreateNewProjectItem(ProjectItem* parentItem, BPath path);

private:
	ProjectOutlineListView*	fOutlineListView;
	GenioWatchingFilter*	fGenioWatchingFilter;

	//TODO: remove this and use a std::vector<std::pair or similar.
	BObjectList<ProjectFolder>	fProjectList;
	BObjectList<ProjectItem>	fProjectProjectItemList;
};
