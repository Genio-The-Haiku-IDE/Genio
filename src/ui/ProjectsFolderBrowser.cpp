/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

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

#include "Log.h"
#include "ProjectsFolderBrowser.h"
#include "ProjectFolder.h"
#include "ProjectItem.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectsFolderBrowser"

ProjectsFolderBrowser::ProjectsFolderBrowser():
	BOutlineListView("ProjectsFolderOutline", B_SINGLE_SELECTION_LIST)
{
	fProjectMenu = new BPopUpMenu("ProjectMenu", false, false);

	fCloseProjectMenuItem = new BMenuItem(B_TRANSLATE("Close project"),
		new BMessage(MSG_PROJECT_MENU_CLOSE));
	fSetActiveProjectMenuItem = new BMenuItem(B_TRANSLATE("Set Active"),
		new BMessage(MSG_PROJECT_MENU_SET_ACTIVE));
	fDeleteFileProjectMenuItem = new BMenuItem(B_TRANSLATE("Delete file"),
		new BMessage(MSG_PROJECT_MENU_DELETE_FILE));
	fOpenFileProjectMenuItem = new BMenuItem(B_TRANSLATE("Open file"),
		new BMessage(MSG_PROJECT_MENU_OPEN_FILE));
	fShowInTrackerProjectMenuItem = new BMenuItem(B_TRANSLATE("Show in Tracker"),
		new BMessage(MSG_PROJECT_MENU_SHOW_IN_TRACKER));
	fOpenTerminalProjectMenuItem = new BMenuItem(B_TRANSLATE("Open Terminal"),
		new BMessage(MSG_PROJECT_MENU_OPEN_TERMINAL));

	fProjectMenu->AddItem(fCloseProjectMenuItem);
	fProjectMenu->AddItem(fSetActiveProjectMenuItem);
	fProjectMenu->AddSeparatorItem();
	fProjectMenu->AddItem(fOpenFileProjectMenuItem);
	fProjectMenu->AddItem(fDeleteFileProjectMenuItem);
	fProjectMenu->AddSeparatorItem();
	fProjectMenu->AddItem(fShowInTrackerProjectMenuItem);
	fProjectMenu->AddItem(fOpenTerminalProjectMenuItem);
	
	SetInvocationMessage(new BMessage(MSG_PROJECT_MENU_OPEN_FILE));
	
}

 
ProjectsFolderBrowser::~ProjectsFolderBrowser()
{
	delete fProjectMenu;
}

// TODO:
// Optimize the search under a specific ProjectItem, tipically a 
// superitem (ProjectFolder)
ProjectItem*	
ProjectsFolderBrowser::FindProjectItem(BString const& path)
{
	ProjectItem *item = nullptr;
	int32 countItems = FullListCountItems();
	for(int32 i=0; i<countItems; i++)
	{
		item = (ProjectItem *)FullListItemAt(i);
		if (item->GetSourceItem()->Path() == path)
			return item;
	}
	return item;
}

void				
ProjectsFolderBrowser::_UpdateNode(BMessage* message)
{
	int32 opCode;
	if (message->FindInt32("opcode", &opCode) != B_OK)
		return;
	LogDebug("opcode %d", opCode);
	
	switch (opCode) {
		case 1:
		{
			BString spath;
			if (message->FindString("path", &spath) == B_OK) {
				BPath path(spath.String());
				BPath parent;
				path.GetParent(&parent);
				
				ProjectItem *parentItem = FindProjectItem(parent.Path());
				ProjectFolder *projectFolder = parentItem->GetSourceItem()->GetProjectFolder();
				SourceItem *sourceItem = new SourceItem(path.Path());
				sourceItem->SetProjectFolder(projectFolder);
				ProjectItem *newItem = new ProjectItem(sourceItem);
				
				LogDebug("ProjectFolder %s", projectFolder->Path().String());
				LogDebug("parent %s", parentItem->GetSourceItem()->Path().String());
				LogDebug("path %s",  newItem->GetSourceItem()->Path().String());
				
				bool status = AddUnder(newItem,parentItem);
				if (status) {
					LogDebug("AddUnder(newItem,item)");
					SortItemsUnder(parentItem, true, ProjectsFolderBrowser::_CompareProjectItems);
					Collapse(newItem);
				}
				
			}
			break;
		}
		case 2:
		{
			BString spath;
			
			if (message->FindString("path", &spath) != B_OK)
				break;

			LogDebug("path %s",  spath.String());
			ProjectItem *item = FindProjectItem(spath);
			RemoveItem(item);
			break;
		}
		case 3:
		{
			BString spath;
			bool removed;
			
			if (message->FindBool("removed", &removed) == B_OK) {
				if (message->FindString("from path", &spath) == B_OK) {			
					LogDebug("from path %s",  spath.String());
					ProjectItem *item = FindProjectItem(spath);
					RemoveItem(item);
				}
			} else {
				// rename
				BString oldName, newName;
				BString oldPath, newPath;
				if (message->FindString("from name", &oldName) == B_OK) {
					if (message->FindString("name", &newName) == B_OK) {
						if (message->FindString("from path", &oldPath) == B_OK) {
							if (message->FindString("path", &newPath) == B_OK) {
								ProjectItem *item = FindProjectItem(oldPath);
								item->SetText(newName);
								item->GetSourceItem()->Rename(newPath);
								Invalidate();
							}
						}
					}
				}
			}
			break;
		}	
		default:
		break;
	}	
		
}

void				
ProjectsFolderBrowser::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case B_PATH_MONITOR:
			message->PrintToStream(); 			  
			_UpdateNode(message);
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
		case MSG_PROJECT_MENU_ADD_ITEM: 
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
	};*/
	BOutlineListView::MessageReceived(message);
}

void
ProjectsFolderBrowser::_ShowProjectItemPopupMenu(BPoint where)
{
	ProjectItem*  projectItem = GetCurrentProjectItem();
		
	if (!projectItem)
		return;
	
	fCloseProjectMenuItem->SetEnabled(false);
	fSetActiveProjectMenuItem->SetEnabled(false);
	fDeleteFileProjectMenuItem->SetEnabled(false);
	fOpenFileProjectMenuItem->SetEnabled(false);
	fShowInTrackerProjectMenuItem->SetEnabled(true);
	fOpenTerminalProjectMenuItem->SetEnabled(false);
	
	ProjectFolder *project = _GetProjectFromItem(projectItem);
	if (project==nullptr)
		return;
	
	if (projectItem->GetSourceItem()->Type() != 
		SourceItemType::FileItem)
		fOpenTerminalProjectMenuItem->SetEnabled(true);
	
	if (projectItem->GetSourceItem()->Type() == 
		SourceItemType::ProjectFolderItem)
	{
		fCloseProjectMenuItem->SetEnabled(true);
		if (!project->Active()) {
			fSetActiveProjectMenuItem->SetEnabled(true);
		} else {
			// Active building project: return
			if (fIsBuilding)
				return;
		}

	} else {
		fDeleteFileProjectMenuItem->SetEnabled(true);
		fOpenFileProjectMenuItem->SetEnabled(true);
	}

	fProjectMenu->Go(ConvertToScreen(where), true, true, false);
}

ProjectItem*
ProjectsFolderBrowser::GetCurrentProjectItem()
{
	int32 selection = CurrentSelection();
	if (selection < 0)
		return nullptr;
		
	return dynamic_cast<ProjectItem*>(ItemAt(selection));
}


ProjectFolder*
ProjectsFolderBrowser::GetProjectFromCurrentItem()
{
	return _GetProjectFromItem(GetCurrentProjectItem());
}

BString const	
ProjectsFolderBrowser::GetCurrentProjectFileFullPath()
{
	ProjectItem* selectedProjectItem = GetCurrentProjectItem();
	if (selectedProjectItem->GetSourceItem()->Type() == SourceItemType::FileItem)
		return selectedProjectItem->GetSourceItem()->Path();
	else
		return "";
}

ProjectFolder*
ProjectsFolderBrowser::_GetProjectFromItem(ProjectItem* item)
{
	if (!item)
		return nullptr;
		
	ProjectFolder *project;
	if (item->GetSourceItem()->Type() == SourceItemType::ProjectFolderItem)
	{
		project = (ProjectFolder *)item->GetSourceItem();
	} else {
		project = (ProjectFolder *)item->GetSourceItem()->GetProjectFolder();
	}
	
	return project;	
}

void
ProjectsFolderBrowser::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();
	
	fProjectMenu->SetTargetForItems(Window());
	BOutlineListView::SetTarget((BHandler*)this, Window());
}

void
ProjectsFolderBrowser::MouseDown(BPoint where)
{	
	BOutlineListView::MouseDown(where);
	
	if (IndexOf(where) >= 0) {
		int32 buttons = -1;

		BMessage* message = Looper()->CurrentMessage();
		 if (message != NULL)
			 message->FindInt32("buttons", &buttons);
		
		if (buttons == B_MOUSE_BUTTON(2)) {
			_ShowProjectItemPopupMenu(where);			
		}
	}
}

void
ProjectsFolderBrowser::ProjectFolderDepopulate(ProjectFolder* project)
{	
	BPrivate::BPathMonitor::StopWatching(BMessenger(this));
	RemoveItem(GetCurrentProjectItem());
	Invalidate();
}

void
ProjectsFolderBrowser::ProjectFolderPopulate(ProjectFolder* project)
{
	ProjectItem *projectItem = NULL;
	_ProjectFolderScan(projectItem, project->Path(), project);
	SortItemsUnder(projectItem, false, ProjectsFolderBrowser::_CompareProjectItems);
	Invalidate();
	
	BPrivate::BPathMonitor::StartWatching(project->Path(),
			B_WATCH_RECURSIVELY, BMessenger(this));
}

void
ProjectsFolderBrowser::_ProjectFolderScan(ProjectItem* item, BString const& path, ProjectFolder *projectFolder)
{
	char name[B_FILE_NAME_LENGTH];
	BEntry nextEntry;
	BEntry entry(path);
	entry.GetName(name);	
			
	ProjectItem *newItem;

	if (item!=NULL) {
		SourceItem *sourceItem = new SourceItem(path);
		sourceItem->SetProjectFolder(projectFolder);
		newItem = new ProjectItem(sourceItem);
		AddUnder(newItem,item);
		Collapse(newItem);
	} else {
		newItem = item = new ProjectItem(projectFolder);
		AddItem(newItem);
	}
	
	if (entry.IsDirectory())
	{
		BPath _currentPath;
		BDirectory dir(&entry);
		while(dir.GetNextEntry(&nextEntry, false)!=B_ENTRY_NOT_FOUND)
		{
			nextEntry.GetPath(&_currentPath);
			_ProjectFolderScan(newItem,_currentPath.Path(), projectFolder);
		}
	}
}

int
ProjectsFolderBrowser::_CompareProjectItems(const BListItem* a, const BListItem* b)
{
	if (a == b)
		return 0;

	const ProjectItem* A = dynamic_cast<const ProjectItem*>(a);
	const ProjectItem* B = dynamic_cast<const ProjectItem*>(b);
	const char* nameA = A->Text();
	SourceItem *itemA = A->GetSourceItem();
	const char* nameB = B->Text();
	SourceItem *itemB = B->GetSourceItem();
	
	if (itemA->Type()==SourceItemType::FolderItem && itemB->Type()==SourceItemType::FileItem) {
		return -1;
	}
	
	if (itemA->Type()==SourceItemType::FileItem && itemB->Type()==SourceItemType::FolderItem) {
		return 1;
	}
		
	if (nameA == NULL) {
		return 1;
	}
		
	if (nameB == NULL) {
		return -1;
	}

	// Natural order sort
	if (nameA != NULL && nameB != NULL)
		return BPrivate::NaturalCompare(nameA, nameB);
		
	return 0;
}