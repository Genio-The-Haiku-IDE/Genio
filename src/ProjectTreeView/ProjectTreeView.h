/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_TREE_VIEW_H
#define PROJECT_TREE_VIEW_H

#include <FilePanel.h>
#include <Notification.h>
#include <OutlineListView.h>
#include <ObjectList.h>
#include <SupportDefs.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

#include <filesystem>


#include "FileTreeItem.h"
#include "NodeMonitor.h"


class ScanRefFilter : public BRefFilter {
public:
							ScanRefFilter(const char* base_path);
							~ScanRefFilter();
				
	void					AddPath(std::filesystem::path relative_path);
	void					RemovePath(std::filesystem::path relative_path);
				
	bool					Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat,
									const char* filetype);

private:
	// std::list<entry_ref*>	fExcludedList;
	std::list<std::filesystem::path>	fExcludedList;
	std::filesystem::path				fBasePath;
};


class ProjectTreeView: public BView, INodeMonitorHandler {
public:
	enum ViewMode {
		FILE_VIEW_MODE,
		DIRECTORY_VIEW_MODE
	};

	enum SelectionMode {
		SINGLE_SELECTION_MODE,
		MULTIPLE_SELECTION_MODE
	};
	
	enum ProjectTreeMessage {
		FILE_SEARCH_MSG = 'ptfs'
	};
	
public:
							ProjectTreeView(const char* name, 
											uint32 flags = B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE, 
											BLayout* layout = NULL);
							~ProjectTreeView();
						
	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* msg);
	
	void 					SetNodeMonitor();
	
	status_t				AddRootItem(const entry_ref& directory, ScanRefFilter* filter = nullptr);
	status_t				RemoveRootItem(const entry_ref* ref);
	void					OnRootItemAdded(const entry_ref* ref);
	
	status_t				ActivateRootItem(const entry_ref& ref);
	status_t				DeactivateRootItem(const entry_ref& ref);
	
	void					Refresh(const entry_ref& ref);
	void					RefreshAll();
	void					SetFilter(const entry_ref& ref, ScanRefFilter* filter);
	void					ResetFilter(const entry_ref& ref, ScanRefFilter* filter);
	
	void					FilterAllItemsByText(const BString& search);
	
	void					MoveUnder(BListItem* item, BListItem* superitem, 
										bool moveChildren = false);
	bool					AddItem(BListItem *item);
	bool					AddUnder(BListItem *item, BListItem *superItem);
	bool					RemoveItem(BListItem *item);
	void					SortTree(BListItem *item);
	void					Collapse(BListItem *item);
	FileTreeItem*			CreateItem(const entry_ref* ref);
	
	virtual void 			OnCreated(entry_ref *ref) const;
	virtual void 			OnRemoved(entry_ref *ref) const;
	virtual void 			OnMoved(entry_ref *origin, entry_ref *destination) const;
	virtual void 			OnStatChanged(entry_ref *ref) const;
	
private:
										
	BListItem*				_FindItemByRef(const entry_ref& ref);
	
	int32					_ScanThread(const entry_ref& ref);
	FileTreeItem*			_RecursiveScan(const entry_ref& ref, FileTreeItem* item,
											const int totalEntries, int& entryCount,
											BNotification& notification);
	int						_CountEntries(const entry_ref& ref);
	
	static	int				_CompareProjectItems(const BListItem* a, const BListItem* b);

private:
	BOutlineListView*		fFileTreeView;
	BTextControl*			fFileSearchControl;
	NodeMonitor*			fNodeMonitor;
};


#endif // PROJECT_TREE_VIEW_H