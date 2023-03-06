/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ProjectsFolderBrowser.h"
#include <Window.h>
#include <Catalog.h>
#include <MenuItem.h>
#include "ProjectFolder.h"
#include "ProjectItem.h"
#include <stdio.h>
#include "Log.h"

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

void				
ProjectsFolderBrowser::MessageReceived(BMessage* message)
{
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
