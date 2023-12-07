/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Parts are taken from the TemplatesMenu class from Haiku (Tracker) under the 
 * Open Tracker Licence 
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */

#include "TemplatesMenu.h"

#include <Application.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Mime.h>
#include <MimeType.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>

#include <cstdio>

#include "IconMenuItem.h"
#include "MimeType.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TemplatesMenu"

// TODO: This is duplicated between here and IconCache.cpp
#define DIR_FILETYPE "application/x-vnd.Be-directory"

const char* kNewFolderLabel = "New folder";

TemplatesMenu::TemplatesMenu(BHandler *target, const char* label, 
								BMessage *message, BMessage *show_template_message,
								const BString& defaultDirectory, 
								const BString&  userDirectory,
								ViewMode mode, 
								bool showNewFolder)
	:
	BMenu(label),
	fTarget(target),
	fOpenItem(NULL),
	fMessage(message),
	fShowTemplateMessage(show_template_message),
	fViewMode(mode),
	fDefaultDirectory(defaultDirectory),
	fUserDirectory(userDirectory),
	fShowNewFolder(showNewFolder),
	fEnableNewFolder(true),
	fShowTemplatesDirectory(fShowTemplateMessage!=nullptr ? true : false)
{
}


TemplatesMenu::~TemplatesMenu()
{
}


void
TemplatesMenu::AttachedToWindow()
{
	_BuildMenu();
	BMenu::AttachedToWindow();
	SetTargetForItems(fTarget);
}


status_t
TemplatesMenu::SetTargetForItems(BHandler* target)
{
	status_t result = BMenu::SetTargetForItems(target);
	if (fOpenItem)
		fOpenItem->SetTarget(target);

	return result;
}


void
TemplatesMenu::UpdateMenuState()
{
	_BuildMenu();
}


void
TemplatesMenu::SetViewMode(ViewMode mode, bool enableNewFolder) 
{ 
	fViewMode = mode; 
	fEnableNewFolder = enableNewFolder;
}

// BMessages are passed in the constructor for each of the events:
// * Create new item
// * Show user template folder
// WARNING: Only the what field of the passed message is used to always generate a new message
//
// * Create new item -> New (empty) folder
// what: specified by the caller in the passed BMessage
// type (String): "new_folder"
//
// * Create new item -> New folder template (typically used for project templates)
// what: specified by the caller in the passed BMessage
// type (String): "new_folder_template"
// refs (entry_ref): the entry_ref of the folder
//
// * Create new item -> New folder template (typically used for file templates)
// what: specified by the caller in the passed BMessage
// type (String): "new_file_template"
// refs (entry_ref): the entry_ref of the folder
//
// * Show user template folder -> Send a message to the target to show the user template folder
// (typically /boot/home/config/settings/Genio/templates)
// what: specified by the caller in the passed BMessage
// refs (entry_ref): the entry_ref of the folder
//
bool
TemplatesMenu::_BuildMenu()
{
	// clear everything...
	fOpenItem = nullptr;
	int32 count = CountItems();
	while (count--)
		delete RemoveItem((int32)0);

	if (fShowNewFolder) {
		// Always create a new message
		BMessage *message = new BMessage(fMessage->what);
		message->AddString("type","new_folder");
		// add the folder
		IconMenuItem* menuItem = new IconMenuItem(B_TRANSLATE(kNewFolderLabel),
			message, DIR_FILETYPE, B_MINI_ICON);
		menuItem->SetEnabled(fEnableNewFolder);
		AddItem(menuItem);
		menuItem->SetShortcut('N', 0);
		AddSeparatorItem();
	}
	
	_BuildTemplateItems(fDefaultDirectory);
	AddSeparatorItem();
	_BuildTemplateItems(fUserDirectory);

	if (fShowTemplatesDirectory) {
		AddSeparatorItem();

		// this is the message sent to open the templates folder
		BMessage *template_message = new BMessage(fShowTemplateMessage->what);
		entry_ref dirRef;
		BDirectory userTemplateDirectory(fUserDirectory);
		BEntry entry;
		if (userTemplateDirectory.GetEntry(&entry) == B_OK)
			entry.GetRef(&dirRef);
		template_message->AddRef("refs", &dirRef);

		// add item to show templates folder
		fOpenItem = new BMenuItem(B_TRANSLATE("Edit user templates" B_UTF8_ELLIPSIS),
			template_message);
		AddItem(fOpenItem);
		if (dirRef == entry_ref())
			fOpenItem->SetEnabled(false);
	}

	return count > 0;
}


void
TemplatesMenu::_BuildTemplateItems(const BString& directory)
{
	// the templates folder
	BDirectory templatesDir(directory);
	BEntry entry;
	bool itemEnabled = true;

	while (templatesDir.GetNextEntry(&entry, true) == B_OK) {
		BNode node(&entry); 
		BNodeInfo nodeInfo(&node);
		
		// ViewMode can be changed at any time before the menu is invoked to show/hide folder or
		// directories or both depending on the context and the item selected to perform the 
		// operation on
		if (fViewMode == DIRECTORY_VIEW_MODE && !entry.IsDirectory())
			break;
		if (fViewMode == FILE_VIEW_MODE && entry.IsDirectory())
			break;
		if (fViewMode == DISABLE_FILES_VIEW_MODE && !entry.IsDirectory())
			itemEnabled = false;
		if (fViewMode == DISABLE_DIRECTORIES_VIEW_MODE && entry.IsDirectory())
			itemEnabled = false;

		char fileName[B_FILE_NAME_LENGTH];
		entry.GetName(fileName);
		if (nodeInfo.InitCheck() == B_OK) {
			char mimeType[B_MIME_TYPE_LENGTH];
			nodeInfo.GetType(mimeType);

			BMimeType mime(mimeType);
			if (mime.IsValid()) {
				entry_ref ref;
				entry.GetRef(&ref);

				BMessage *message = new BMessage(fMessage->what);
				message->AddRef("refs", &ref);
				if (entry.IsDirectory())
					message->AddString("type","new_folder_template");
				else
					message->AddString("type","new_file_template");
				auto item = new IconMenuItem(fileName, message, &nodeInfo, B_MINI_ICON);
				item->SetEnabled(itemEnabled);
				AddItem(item);
			}
		}
	}
}
