/*
 * Copyright 2023, Andrea Anzani 
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <MenuItem.h>
#include <Mime.h>
#include <Path.h>
#include <PathMonitor.h>
#include <Window.h>
#include <NaturalCompare.h>

#include <stdio.h>

#include "GenioWindowMessages.h"
#include "GenioWindow.h"
#include "Log.h"
#include "ProjectsFolderBrowser.h"
#include "ProjectFolder.h"
#include "ProjectItem.h"
#include "Utils.h"

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
	fFileNewProjectMenuItem = new TemplatesMenu(this, B_TRANSLATE("New"),
		MSG_PROJECT_MENU_NEW_FILE);
	fDeleteFileProjectMenuItem = new BMenuItem(B_TRANSLATE("Delete file"),
		new BMessage(MSG_PROJECT_MENU_DELETE_FILE));
	fOpenFileProjectMenuItem = new BMenuItem(B_TRANSLATE("Open file"),
		new BMessage(MSG_PROJECT_MENU_OPEN_FILE));
	fRenameFileProjectMenuItem = new BMenuItem(B_TRANSLATE("Rename file"),
		new BMessage(MSG_PROJECT_MENU_RENAME_FILE));
	fShowInTrackerProjectMenuItem = new BMenuItem(B_TRANSLATE("Show in Tracker"),
		new BMessage(MSG_PROJECT_MENU_SHOW_IN_TRACKER));
	fOpenTerminalProjectMenuItem = new BMenuItem(B_TRANSLATE("Open Terminal"),
		new BMessage(MSG_PROJECT_MENU_OPEN_TERMINAL));

	fProjectMenu->AddItem(fCloseProjectMenuItem);
	fProjectMenu->AddItem(fSetActiveProjectMenuItem);
	fProjectMenu->AddSeparatorItem();
	fProjectMenu->AddItem(fFileNewProjectMenuItem);
	fProjectMenu->AddItem(fOpenFileProjectMenuItem);
	fProjectMenu->AddItem(fDeleteFileProjectMenuItem);
	fProjectMenu->AddItem(fRenameFileProjectMenuItem);
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
	int32 countItems = FullListCountItems();
	for(int32 i=0; i<countItems; i++)
	{
		ProjectItem *item = (ProjectItem *)FullListItemAt(i);
		if (item->GetSourceItem()->Path() == path)
			return item;
	}
	return nullptr;
}

ProjectItem*	
ProjectsFolderBrowser::_CreateNewProjectItem(ProjectItem* parentItem, BPath path)
{
	ProjectFolder *projectFolder = parentItem->GetSourceItem()->GetProjectFolder();
	SourceItem *sourceItem = new SourceItem(path.Path());
	sourceItem->SetProjectFolder(projectFolder);
	return new ProjectItem(sourceItem);
}

ProjectItem*	
ProjectsFolderBrowser::_CreatePath(BPath pathToCreate)
{
	LogTrace("Create path for %s", pathToCreate.Path());
	ProjectItem *item = FindProjectItem(pathToCreate.Path());
	
	if (!item) {
		LogTrace("Can't find path %s", pathToCreate.Path());
		BPath parent;
		if (pathToCreate.GetParent(&parent) == B_OK)
		{
			ProjectItem* parentItem = _CreatePath(parent);
			LogTrace("Creating path %s", pathToCreate.Path());
			ProjectItem* newItem = _CreateNewProjectItem(parentItem, pathToCreate);// new ProjectItem(sourceItem);

			if (AddUnder(newItem,parentItem)) {
				LogDebugF("AddUnder(%s,%s) (Parent %s)", newItem->Text(), parentItem->Text(), parent.Path());
				SortItemsUnder(parentItem, true, ProjectsFolderBrowser::_CompareProjectItems);
				Collapse(newItem);
			}
			return newItem;
		}
	}
	LogTrace("Found path [%s]", pathToCreate.Path());
	return item;		
}


void				
ProjectsFolderBrowser::_UpdateNode(BMessage* message)
{
	int32 opCode;
	if (message->FindInt32("opcode", &opCode) != B_OK)
		return;
	BString watchedPath;
	if (message->FindString("watched_path", &watchedPath) != B_OK)
		return;
	
	switch (opCode) {
		case B_ENTRY_CREATED:
		{
			BString spath;
			if (message->FindString("path", &spath) == B_OK) {
				BPath path(spath.String());
				_CreatePath(path);
			}
			break;
		}
		case B_ENTRY_REMOVED:
		{
			BString spath;
			
			if (message->FindString("path", &spath) != B_OK)
				break;

			LogDebug("path %s",  spath.String());
			ProjectItem *item = FindProjectItem(spath);
			if (!item) {
					LogError("Can't find an item to remove [%s]", spath.String());
					return;
			}
			if (item->GetSourceItem()->Type() == 
				SourceItemType::ProjectFolderItem)
			{
				if (LockLooper()) {
					Select(IndexOf(item));
					Window()->PostMessage(MSG_PROJECT_MENU_CLOSE);
				
					// It seems not possible to track the project folder to the new
					// location outside of the watched path. So we close the project 
					// and warn the user
					auto alert = new BAlert("ProjectFolderChanged",
						B_TRANSLATE("The project folder has been deleted or moved to another location and it will be closed and unloaded from the workspace."),
						B_TRANSLATE("OK"), NULL, NULL,
						B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
						alert->Go();
				
					UnlockLooper();
				}
			} else {
				RemoveItem(item);
				SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
			}
			break;
		}
		case B_ENTRY_MOVED:
		{
			BString spath;			
			// An item moved outside of the project folder
			if (message->GetBool("removed", false)) {
				if (message->FindString("from path", &spath) == B_OK) {			
					LogDebug("from path %s",  spath.String());
					ProjectItem *item = FindProjectItem(spath);
					if (!item) {
						LogError("Can't find an item to move [%s]", spath.String());
						return;
					}
					// the project folder is being renamed
					if (item->GetSourceItem()->Type() == 
						SourceItemType::ProjectFolderItem)
					{
						if (LockLooper()) {
							Select(IndexOf(item));
							Window()->PostMessage(MSG_PROJECT_MENU_CLOSE);
						
							auto alert = new BAlert("ProjectFolderChanged",
							B_TRANSLATE("The project folder has been renamed. It will be closed and reopened automatically."),
								B_TRANSLATE("OK"), NULL, NULL,
								B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
							alert->Go();
						
							// reopen project under the new name or location
							entry_ref ref;
							if (message->FindInt64("to directory", &ref.directory) == B_OK) {
								const char *name;
								message->FindInt32("device", &ref.device);
								message->FindString("name", &name);
								ref.set_name(name);
								auto msg = new BMessage(MSG_PROJECT_FOLDER_OPEN);
								msg->AddRef("refs", &ref);
								Window()->PostMessage(msg);
							}
							UnlockLooper();
						}
					} else {
						RemoveItem(item);
						SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
					}
				}
			} else {
				BString oldName, newName;
				BString oldPath, newPath;
				
				if (message->GetBool("added")) {
					if (message->FindString("path", &newPath) == B_OK) {
						if (message->FindString("name", &newName) == B_OK) {
							const BPath destination(newPath);
							if (BEntry(newPath).IsDirectory()) {
								
								// a new folder moved inside the project.
								
								//ensure we have a parent								
								BPath parent;
								destination.GetParent(&parent);
								ProjectItem *parentItem = _CreatePath(parent);
								
								// recursive parsing!
								_ProjectFolderScan(parentItem, newPath, parentItem->GetSourceItem()->GetProjectFolder());
								SortItemsUnder(parentItem, false, ProjectsFolderBrowser::_CompareProjectItems);
							}
							else {	//Plain file.
								_CreatePath(destination);
							}
					}
				  }
				} else 	if (message->FindString("from name", &oldName) == B_OK) {
					if (message->FindString("name", &newName) == B_OK) {
						if (message->FindString("from path", &oldPath) == B_OK) {
							if (message->FindString("path", &newPath) == B_OK) {
								const BPath bp_oldPath(oldPath.String());
								BPath bp_oldParent;
								bp_oldPath.GetParent(&bp_oldParent);
								
								const BPath bp_newPath(newPath.String());
								BPath bp_newParent;
								bp_newPath.GetParent(&bp_newParent);
								
								// If the path remains the same except the leaf
								// then the item is being RENAMED
								// if the path changes then the item is being MOVED
								if (bp_oldParent == bp_newParent) {
									ProjectItem *item = FindProjectItem(oldPath);
									if (!item) {
										LogError("Can't find an item to move oldPath[%s] -> newPath[%s]", oldPath.String(), newPath.String());
										return;
									}
									item->SetText(newName);
									item->GetSourceItem()->Rename(newPath);
									SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
								} else {
									ProjectItem *item = FindProjectItem(oldPath);
									if (!item) {
										LogError("Can't find an item to move oldPath [%s]", oldPath.String());
										return;
									}
									ProjectItem *destinationItem = FindProjectItem(bp_newParent.Path());
									if (!item) {
										LogError("Can't find an item to move newParent [%s]", bp_newParent.Path());
										return;
									}
									bool status;
										
									status = RemoveItem(item);
									if (status) {
										SortItemsUnder(Superitem(item), true, ProjectsFolderBrowser::_CompareProjectItems);
										
										ProjectItem *newItem = _CreateNewProjectItem(item, bp_newPath);
										
										status = AddUnder(newItem, destinationItem);
										if (status) {
											SortItemsUnder(destinationItem, true, ProjectsFolderBrowser::_CompareProjectItems);
										}
									}
								}
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
		case B_PATH_MONITOR: {
			if (Logger::IsDebugEnabled())
				message->PrintToStream(); 			  
			_UpdateNode(message);
		}
		break;
		case MSG_PROJECT_MENU_OPEN_FILE: 
		{
			//message->PrintToStream();
			int32 index = -1;
			if (message->FindInt32("index", &index) != B_OK) {
				LogError("(MSG_PROJECT_MENU_OPEN_FILE) Can't find index!");
				return;
			}
			
			ProjectItem* item = dynamic_cast<ProjectItem*>(ItemAt(index));
			if (!item) {
				LogError("(MSG_PROJECT_MENU_OPEN_FILE) Can't find item at index %d", index);
				return;
			}
			if (item->GetSourceItem()->Type() != SourceItemType::FileItem) {
				LogDebug("(MSG_PROJECT_MENU_OPEN_FILE) Invoking a non FileItem (%s)", item->GetSourceItem()->Name().String());
				return;
			}
			
			BEntry entry(item->GetSourceItem()->Path());
			entry_ref ref;
			if (entry.GetRef(&ref) != B_OK) {
				LogError("(MSG_PROJECT_MENU_OPEN_FILE) Invalid entry_ref for [%s]", item->GetSourceItem()->Path().String());
				return;
			}
			
			BMessage msg(B_REFS_RECEIVED);
			msg.AddRef("refs", &ref);
			msg.AddBool("openWithPreferred", true);
			Window()->PostMessage(&msg);
			return;
		}
		break;
		case MSG_PROJECT_MENU_DO_RENAME_FILE:
		{
			BString newName;
			if (message->FindString("_value", &newName) == B_OK)
				_RenameCurrentSelectedFile(newName);
		}
		break;
		default:
		break;	
	}
	
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
	fFileNewProjectMenuItem->SetEnabled(false);
	fDeleteFileProjectMenuItem->SetEnabled(false);
	fOpenFileProjectMenuItem->SetEnabled(false);
	fRenameFileProjectMenuItem->SetEnabled(false);
	fShowInTrackerProjectMenuItem->SetEnabled(false);
	fOpenTerminalProjectMenuItem->SetEnabled(false);
	
	ProjectFolder *project = _GetProjectFromItem(projectItem);
	if (project==nullptr)
		return;
	
	if (projectItem->GetSourceItem()->Type() == SourceItemType::FileItem) {
		fCloseProjectMenuItem->SetEnabled(false);
		fSetActiveProjectMenuItem->SetEnabled(false);
		fFileNewProjectMenuItem->SetEnabled(false);
		fDeleteFileProjectMenuItem->SetEnabled(true);
		fRenameFileProjectMenuItem->SetEnabled(true);
		fOpenFileProjectMenuItem->SetEnabled(true);
		fShowInTrackerProjectMenuItem->SetEnabled(true);
		fOpenTerminalProjectMenuItem->SetEnabled(false);
	}
	
	if (projectItem->GetSourceItem()->Type() == SourceItemType::ProjectFolderItem)
	{
		fCloseProjectMenuItem->SetEnabled(true);
		if (!project->Active()) {
			fSetActiveProjectMenuItem->SetEnabled(true);
		} else {
			// Active building project: return
			if (fIsBuilding)
				return;
		}
		
		fFileNewProjectMenuItem->SetEnabled(true);
		fDeleteFileProjectMenuItem->SetEnabled(false);
		fRenameFileProjectMenuItem->SetEnabled(false);
		fOpenFileProjectMenuItem->SetEnabled(false);
		fShowInTrackerProjectMenuItem->SetEnabled(true);
		fOpenTerminalProjectMenuItem->SetEnabled(true);

	} 

	if (projectItem->GetSourceItem()->Type() == SourceItemType::FolderItem) {
		fCloseProjectMenuItem->SetEnabled(false);
		fSetActiveProjectMenuItem->SetEnabled(false);
		fFileNewProjectMenuItem->SetEnabled(true);
		fDeleteFileProjectMenuItem->SetEnabled(true);
		fRenameFileProjectMenuItem->SetEnabled(true);
		fOpenFileProjectMenuItem->SetEnabled(false);
		fShowInTrackerProjectMenuItem->SetEnabled(true);
		fOpenTerminalProjectMenuItem->SetEnabled(true);
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
	// if (selectedProjectItem->GetSourceItem()->Type() == SourceItemType::FileItem)
		return selectedProjectItem->GetSourceItem()->Path();
	// else
		// return "";
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

status_t
ProjectsFolderBrowser::_RenameCurrentSelectedFile(const BString& new_name)
{
	status_t status;
	ProjectItem *item = GetCurrentProjectItem();
	if (item) {
		BPath path(item->GetSourceItem()->Path());
		BPath newPath;
		path.GetParent(&newPath);
		newPath.Append(new_name);
		BEntry entry(item->GetSourceItem()->Path());
		status = entry.Rename(newPath.Path(), false);
	}
	return status;
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
	ProjectItem*	listItem = FindProjectItem(project->Path());
	if (listItem)
		RemoveItem(listItem);
	else
		LogErrorF("Can't find ProjectItem for path [%s]", project->Path().String());
	Invalidate();
}

void
ProjectsFolderBrowser::ProjectFolderPopulate(ProjectFolder* project)
{
	ProjectItem *projectItem = NULL;
	_ProjectFolderScan(projectItem, project->Path(), project);
	SortItemsUnder(projectItem, false, ProjectsFolderBrowser::_CompareProjectItems);
	
	update_mime_info(project->Path(), true, false, B_UPDATE_MIME_INFO_NO_FORCE);
	
	Invalidate();
	
	if (BPrivate::BPathMonitor::StartWatching(project->Path(),
			B_WATCH_RECURSIVELY, BMessenger(this)) != B_OK)
	{
		LogError("Can't start PathMonitor!");
	}
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

void
ProjectsFolderBrowser::SelectionChanged() {
	GenioWindow *window = (GenioWindow*)this->Window();
	window->UpdateMenu();
}