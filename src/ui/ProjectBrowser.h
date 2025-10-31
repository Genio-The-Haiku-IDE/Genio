/*
 * Copyright 2023-2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <map>

#include <Locker.h>
#include <View.h>

#include "ProjectFolder.h"
#include "ProjectItem.h"


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

	MSG_BROWSER_SELECT_ITEM				= 'sele',
	MSG_PROJECT_ADD_ITEMS_BATCH			= 'paib',
	MSG_PROJECT_BATCH_SYNC				= 'pbsy'
};

// Batch size for adding items - tuned for performance vs responsiveness
constexpr int32 kProjectItemBatchSize = 100;

class ProjectOutlineListView;
class GenioWatchingFilter;

class ProjectBrowser : public BView {
public:
					 ProjectBrowser();
					 ProjectBrowser(const ProjectBrowser&) = delete;
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

	void			SelectProjectAndScroll(const ProjectFolder*);

	void			SelectItemByRef(const ProjectFolder* project, const entry_ref& ref);
	void			SelectNewItemAndScrollDelayed(const ProjectItem* parent, const entry_ref ref); //ugly name..

	ProjectFolder* 	ProjectFolderPopulate(ProjectFolder* project);
	void			ProjectFolderDepopulate(ProjectFolder* project);

	void			ExpandProjectCollapseOther(const BString& projectName);

	void			InitRename(ProjectItem *item);
private:

	ProjectItem*	GetProjectItemByPath(const BString& path) const;

	ProjectItem*	_ProjectFolderScan(const entry_ref* ref, ProjectItem* parentItem, ProjectFolder *projectFolder = NULL);

	void			_ShowProjectItemPopupMenu(BPoint where);

	ProjectItem*	_CreatePath(BPath pathToCreate);
	void			_RemovePath(BString pathToRemove);
	void			_HandleEntryMoved(BMessage* message);
	void			_UpdateNode(BMessage *message);

	status_t		_RenameCurrentSelectedFile(const BString& newName);

	ProjectItem*	_CreateNewProjectItem(ProjectItem* parentItem, BPath path);

	// Command pattern for batched item insertion
	struct AddItemCommand {
		ProjectItem* item;
		ProjectItem* parent;  // nullptr for root items
		bool shouldCollapse;
	};

	// Batch processing helpers
	void			_AddItemCommandToBatch(ProjectItem* item, ProjectItem* parent, ProjectFolder* project);
	void			_FlushItemBatch(ProjectFolder* project, bool synchronous = false);
	void			_FlushItemBatchLocked(ProjectFolder* project, bool synchronous);
	void			_ProcessItemBatch(BMessage* message);

private:
	ProjectOutlineListView*	fOutlineListView;
	GenioWatchingFilter*	fGenioWatchingFilter;

	ProjectFolderList	fProjectList;
	ProjectItemList		fProjectProjectItemList;

	// Batch processing state (protected by fBatchLock for thread safety)
	// Each project has its own batch of commands to prevent mixing
	BLocker					fBatchLock;
	std::map<ProjectFolder*, std::vector<AddItemCommand>>	fItemBatches;
};
