/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FileTreeView_H
#define FileTreeView_H


#include <SupportDefs.h>
#include <ObjectList.h>
#include <OutlineListView.h>
#include <FilePanel.h>
#include <View.h>

#include <list>

#include "FileTreeItem.h"
#include "NodeMonitor.h"

class FileTreeItem;

class FileTreeView : public BOutlineListView, INodeMonitorHandler {
public:
	enum FileTreeViewMode {
		FILE_VIEW_MODE,
		DIRECTORY_VIEW_MODE
	};

	enum FileTreeSelectionMode {
		SINGLE_SELECTION_MODE,
		MULTIPLE_SELECTION_MODE
	};
	
public:
							FileTreeView(FileTreeViewMode viewMode = FILE_VIEW_MODE,
											FileTreeSelectionMode selectionMode = 
												SINGLE_SELECTION_MODE);
	virtual 				~FileTreeView();
	
	void					MouseDown(BPoint where);
	void					AttachedToWindow();
	void					MessageReceived(BMessage* message);
	
	void					AddRootItem(const BEntry* directory,
										BRefFilter* filter = nullptr,
										bool watch=true);
	void					AddRootItem(const BDirectory* directory,
										BRefFilter* filter = nullptr,
										bool watch=true);
	void					AddRootItem(const entry_ref& directory,
										BRefFilter* filter = nullptr,
										bool watch=true);
	void					AddRootItem(const char* directory,
										BRefFilter* filter = nullptr,
										bool watch=true);
										
	BObjectList<entry_ref*>	GetProjects() const;
	void					RemoveProject(entry_ref&);

	void					Refresh(entry_ref&);
	void					SetFilter(entry_ref&, BRefFilter* filter);
	void					ResetFilter(entry_ref&, BRefFilter* filter);
	
private:
	
	void					_RootItemDepopulate(FileTreeItem* directory);
	void					_RootItemPopulate(const entry_ref& ref);
	void					_RootItemScan(FileTreeItem* item, const entry_ref& ref);
	
	FileTreeItem*			_FindTreeItem(entry_ref& ref);
	static	int				_CompareProjectItems(const BListItem* a, const BListItem* b);
	
	void					_UpdateNode(BMessage* message);
	
	// INodeMonitorHandler
	virtual void 			OnCreated(entry_ref& ref) const {};
	virtual void 			OnRemoved(entry_ref& ref) const {};
	virtual void 			OnMoved(entry_ref& origin, entry_ref& destination) const {};
	virtual void 			OnStatChanged(entry_ref& ref) const {};
	
private:
	
	FileTreeViewMode		fViewMode;
	FileTreeSelectionMode	fSelectionMode;		
};


#endif // FileTreeView_H
