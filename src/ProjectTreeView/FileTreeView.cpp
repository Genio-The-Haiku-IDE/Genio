/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <MenuItem.h>
#include <Path.h>
#include <PathMonitor.h>
#include <Window.h>
#include <NaturalCompare.h>

#include <stdio.h>

#include "FileTreeView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FileTreeView"

FileTreeView::FileTreeView(FileTreeViewMode viewMode, 
							FileTreeSelectionMode selectionMode):
	BOutlineListView("FileTreeView", B_SINGLE_SELECTION_LIST),
	fViewMode(viewMode),
	fSelectionMode(selectionMode)
{
}

 
FileTreeView::~FileTreeView()
{
}

void				
FileTreeView::_UpdateNode(BMessage* message)
{
	// int32 opCode;
	// if (message->FindInt32("opcode", &opCode) != B_OK)
		// return;
	// 
	// switch (opCode) {
		// case B_ENTRY_CREATED:
		// {
			// BString spath;
			// if (message->FindString("path", &spath) == B_OK) {
				// BPath path(spath.String());
				// BPath parent;
				// path.GetParent(&parent);
				// 
				// ProjectItem *parentItem = FindProjectItem(parent.Path());
				// ProjectFolder *projectFolder = parentItem->GetSourceItem()->GetProjectFolder();
				// SourceItem *sourceItem = new SourceItem(path.Path());
				// sourceItem->SetProjectFolder(projectFolder);
				// ProjectItem *newItem = new ProjectItem(sourceItem);
				// 
				// bool status = AddUnder(newItem,parentItem);
				// if (status) {
					// LogDebug("AddUnder(newItem,item)");
					// SortItemsUnder(parentItem, true, ProjectsFolderBrowser::_CompareProjectItems);
					// Collapse(newItem);
				// }
				// 
			// }
			// break;
		// }
		// case B_ENTRY_REMOVED:
		// {
			// BString spath;
			// 
			// if (message->FindString("path", &spath) != B_OK)
				// break;
// 
			// LogDebug("path %s",  spath.String());
			// ProjectItem *item = FindProjectItem(spath);
			// if (item->GetSourceItem()->Type() == 
				// SourceItemType::ProjectFolderItem)
			// {
				// if (LockLooper()) {
					// Select(IndexOf(item));
					// Window()->PostMessage(MSG_PROJECT_MENU_CLOSE);
				// 
					// It seems not possible to track the project folder to the new
					// location outside of the watched path. So we close the project 
					// and warn the user
					// auto alert = new BAlert("ProjectFolderChanged",
						// B_TRANSLATE("The project folder has been deleted or moved to another location and it will be closed and unloaded from the workspace."),
						// B_TRANSLATE("OK"), NULL, NULL,
						// B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
						// alert->Go();
				// 
					// UnlockLooper();
				// }
			// } else {
				// RemoveItem(item);
				// SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
			// }
			// break;
		// }
		// case B_ENTRY_MOVED:
		// {
			// BString spath;
			// bool removed, added;
			// 
			// An item moved outside of the project folder
			// if (message->FindBool("removed", &removed) == B_OK) {
				// if (message->FindString("from path", &spath) == B_OK) {			
					// LogDebug("from path %s",  spath.String());
					// ProjectItem *item = FindProjectItem(spath);
					// 
					// the project folder is being renamed
					// if (item->GetSourceItem()->Type() == 
						// SourceItemType::ProjectFolderItem)
					// {
						// if (LockLooper()) {
							// Select(IndexOf(item));
							// Window()->PostMessage(MSG_PROJECT_MENU_CLOSE);
						// 
							// auto alert = new BAlert("ProjectFolderChanged",
							// B_TRANSLATE("The project folder has been renamed. It will be closed and reopened automatically."),
								// B_TRANSLATE("OK"), NULL, NULL,
								// B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
							// alert->Go();
						// 
							// reopen project under the new name or location
							// entry_ref ref;
							// if (message->FindInt64("to directory", &ref.directory) == B_OK) {
								// const char *name;
								// message->FindInt32("device", &ref.device);
								// message->FindString("name", &name);
								// ref.set_name(name);
								// auto msg = new BMessage(MSG_PROJECT_FOLDER_OPEN);
								// msg->AddRef("refs", &ref);
								// Window()->PostMessage(msg);
							// }
							// UnlockLooper();
						// }
					// } else {
						// RemoveItem(item);
						// SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
					// }
				// }
			// } else {
				// BString oldName, newName;
				// BString oldPath, newPath;
				// 
				// if (message->FindBool("added", &added) == B_OK) {
					// if (message->FindString("path", &newPath) == B_OK) {
						// if (message->FindString("name", &newName) == B_OK) {
							// BPath destination(newPath);
							// BPath parent;
							// destination.GetParent(&parent);
							// 
							// ProjectItem *parentItem = FindProjectItem(parent.Path());
							// ProjectFolder *projectFolder = parentItem->GetSourceItem()->GetProjectFolder();
							// SourceItem *sourceItem = new SourceItem(newPath);
							// sourceItem->SetProjectFolder(projectFolder);
							// ProjectItem *newItem = new ProjectItem(sourceItem);
				// 
							// LogDebug("ProjectFolder %s", projectFolder->Path().String());
							// LogDebug("parent %s", parentItem->GetSourceItem()->Path().String());
							// LogDebug("path %s",  newItem->GetSourceItem()->Path().String());
				// 
							// if (newItem->GetSourceItem()->Type() == SourceItemType::FolderItem)
							// {
								// _ProjectFolderScan(parentItem, newItem->GetSourceItem()->Path(), newItem->GetSourceItem()->GetProjectFolder());
								// SortItemsUnder(parentItem, false, ProjectsFolderBrowser::_CompareProjectItems);
								// Collapse(newItem);
							// } else {
								// bool status = AddUnder(newItem,parentItem);
								// if (status) {
									// SortItemsUnder(parentItem, true, ProjectsFolderBrowser::_CompareProjectItems);
									// Collapse(newItem);
								// }
							// }	
						// }
					// }
				// } else 	if (message->FindString("from name", &oldName) == B_OK) {
					// if (message->FindString("name", &newName) == B_OK) {
						// if (message->FindString("from path", &oldPath) == B_OK) {
							// if (message->FindString("path", &newPath) == B_OK) {
								// BPath bp_oldPath(oldPath.String());
								// BPath bp_oldParent;
								// bp_oldPath.GetParent(&bp_oldParent);
								// 
								// BPath bp_newPath(newPath.String());
								// BPath bp_newParent;
								// bp_newPath.GetParent(&bp_newParent);
								// 
								// If the path remains the same except the leaf
								// then the item is being RENAMED
								// if the path changes then the item is being MOVED
								// if (bp_oldParent == bp_newParent) {
									// ProjectItem *item = FindProjectItem(oldPath);
									// item->SetText(newName);
									// item->GetSourceItem()->Rename(newPath);
									// SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
								// } else {
									// ProjectItem *item = FindProjectItem(oldPath);
									// ProjectItem *destinationItem = FindProjectItem(bp_newParent.Path());
									// bool status;
										// 
									// status = RemoveItem(item);
									// if (status) {
										// SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
										// 
										// ProjectFolder *projectFolder = item->GetSourceItem()->GetProjectFolder();
										// SourceItem *sourceItem = new SourceItem(newPath);
										// sourceItem->SetProjectFolder(projectFolder);
										// ProjectItem *newItem = new ProjectItem(sourceItem);
										// 
										// status = AddUnder(newItem, destinationItem);
										// if (status) {
											// SortItemsUnder(destinationItem, true, ProjectsFolderBrowser::_CompareProjectItems);
										// }
									// }
								// }
							// }
						// }
					// }
				// }
			// }
			// break;
		// }	
		// default:
		// break;
	// }	
		
}

void				
FileTreeView::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case B_PATH_MONITOR: {
			message->PrintToStream(); 			  
			_UpdateNode(message);
		}
		break;
		default:
		break;	
	}
	
	//TODO: move more logic here!
	/*
	switch (message->what)
	{
		case MSG_PROJECT_MENU_CLOSE: 
			message->PrintToStream();
		break;
		case MSG_PROJECT_MENU_SET_ACTIVE: 
			message->PrintToStream();
		break;
		case MSG_PROJECT_MENU_DELETE_FILE: 
			message->PrintToStream();
		break;
		case MSG_PROJECT_MENU_EXCLUDE_FILE: 
			message->PrintToStream();
		break;
		case MSG_PROJECT_MENU_OPEN_FILE: 
			message->PrintToStream();
		break;
		case MSG_PROJECT_MENU_SHOW_IN_TRACKER: 
			message->PrintToStream();
		break;
		case MSG_PROJECT_MENU_OPEN_TERMINAL: 
			message->PrintToStream();
		break;
		
		default:break;
	};
	*/
	
	// BOutlineListView::MessageReceived(message);
}


void
FileTreeView::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();
	
	BOutlineListView::SetTarget((BHandler*)this, Window());
}

void
FileTreeView::MouseDown(BPoint where)
{	
	BOutlineListView::MouseDown(where);
	
	if (IndexOf(where) >= 0) {
		int32 buttons = -1;

		BMessage* message = Looper()->CurrentMessage();
		 if (message != NULL)
			 message->FindInt32("buttons", &buttons);
		
		if (buttons == B_MOUSE_BUTTON(2)) {
			// _ShowProjectItemPopupMenu(where);			
		}
	}
}

void
FileTreeView::_RootItemDepopulate(FileTreeItem* directory)
{	
	// BPrivate::BPathMonitor::StopWatching(BMessenger(this));
	// RemoveItem(GetCurrentProjectItem());
	Invalidate();
}

void
FileTreeView::_RootItemPopulate(const entry_ref& ref)
{
	FileTreeItem *treeItem = NULL;
	_RootItemScan(treeItem, ref);
	AddItem(treeItem);
	SortItemsUnder(treeItem, false, FileTreeView::_CompareProjectItems);
	Invalidate();
	
	// BPrivate::BPathMonitor::StartWatching(project->Path(),
			// B_WATCH_RECURSIVELY, BMessenger(this));
}

void
FileTreeView::_RootItemScan(FileTreeItem* item, const entry_ref& ref)
{
	BEntry entry(&ref);
	
	FileTreeItem *newItem;

	if (item!=nullptr) {
		newItem = new FileTreeItem(ref, this);
		AddUnder(newItem,item);
		Collapse(newItem);
	} else {
		// this is the root directory (superitem)
		newItem = item = new FileTreeItem(ref, this);
		AddItem(newItem);
	}
	
	if (entry.IsDirectory())
	{
		entry_ref nextRef;
		BEntry nextEntry;
		BDirectory dir(&entry);
		while(dir.GetNextEntry(&nextEntry, false)!=B_ENTRY_NOT_FOUND)
		{
			nextEntry.GetRef(&nextRef);
			// _ProjectScan(newItem, &nextRef);
		}
	}
}

int
FileTreeView::_CompareProjectItems(const BListItem* a, const BListItem* b)
{
	if (a == b)
		return 0;

	const FileTreeItem* A = dynamic_cast<const FileTreeItem*>(a);
	const FileTreeItem* B = dynamic_cast<const FileTreeItem*>(b);
	
	if (A->IsDirectory() && !B->IsDirectory()) {
		return -1;
	}
	
	if (!A->IsDirectory() && B->IsDirectory()) {
		return -1;
	}
		
	if (A->Text() == NULL) {
		return 1;
	}
		
	if (B->Text() == NULL) {
		return -1;
	}

	// Natural order sort
	if (A->Text() != NULL && B->Text() != NULL)
		return BPrivate::NaturalCompare(A->Text(), B->Text());
		
	return 0;
}

void					
FileTreeView::AddRootItem(const entry_ref& directory,
							BRefFilter* filter,
							bool watch)
{
	_RootItemPopulate(directory);
}