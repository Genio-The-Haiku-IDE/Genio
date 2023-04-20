/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_TREE_VIEW_H
#define PROJECT_TREE_VIEW_H

#include <FilePanel.h>
#include <Notification.h>
#include <OutlineListView.h>
<<<<<<< HEAD
#include <ObjectList.h>
=======
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
#include <SupportDefs.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

<<<<<<< HEAD
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


=======
#include "FileTreeItem.h"
#include "NodeMonitor.h"

>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
class ProjectTreeView: public BView {
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
	
<<<<<<< HEAD
	status_t				AddRootItem(const entry_ref& directory, ScanRefFilter* filter = nullptr);
=======
	status_t				AddRootItem(const entry_ref& directory, BRefFilter* filter = nullptr);
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
	status_t				RemoveRootItem(const entry_ref& directory);
	
	status_t				ActivateRootItem(const entry_ref& ref);
	status_t				DeactivateRootItem(const entry_ref& ref);
	
	void					Refresh(const entry_ref& ref);
	void					RefreshAll();
<<<<<<< HEAD
	void					SetFilter(const entry_ref& ref, ScanRefFilter* filter);
	void					ResetFilter(const entry_ref& ref, ScanRefFilter* filter);
	
	void					FilterAllItemsByText(const BString& search);
	int						CountEntries(const entry_ref& ref);
	
	void					MoveUnder(BListItem* item, BListItem* superitem, 
										bool moveChildren = false);
	bool					AddItem(BListItem *item);
	bool					AddUnder(BListItem *item, BListItem *superItem);
	bool					RemoveItem(BListItem *item);
	void					SortTree(BListItem *item);
	void					Collapse(BListItem *item);
	FileTreeItem*			CreateItem(const entry_ref* ref);
	
private:
=======
	void					SetFilter(const entry_ref& ref, BRefFilter* filter);
	void					ResetFilter(const entry_ref& ref, BRefFilter* filter);
	
	void					FilterAllItemsByText(const BString& search);
	int						CountEntries(const entry_ref& ref);
	
private:
	void					_MoveUnder(BListItem* item, BListItem* superitem, 
										bool moveChildren = false);
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
										
	BListItem*				_FindItemByRef(const entry_ref& ref);
	
	int32					_ScanThread(const entry_ref& ref);
	FileTreeItem*			_RecursiveScan(const entry_ref& ref, FileTreeItem* item,
											const int totalEntries, int& entryCount,
											BNotification& notification);
	int						_CountEntries(const entry_ref& ref);
	void					_OnRootItemAdded(const entry_ref& ref, BListItem* item);
	
	static	int				_CompareProjectItems(const BListItem* a, const BListItem* b);

private:
	BOutlineListView*		fFileTreeView;
	BTextControl*			fFileSearchControl;
};


#endif // PROJECT_TREE_VIEW_H
