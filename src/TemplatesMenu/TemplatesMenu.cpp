/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 * Parts are taken from the TemplatesMenu class from Haiku (Tracker) under the 
 * Open Tracker Licence 
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */


#include <Application.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <NodeInfo.h>
#include <Locale.h>
#include <Mime.h>
#include <MimeType.h>
#include <Message.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>
#include <MenuItem.h>
#include <stdio.h>

#include "TemplatesMenu.h"
#include "IconMenuItem.h"
#include "MimeTypes.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TemplatesMenu"

const char* kTemplatesMenuName = "";
const char* kTemplatesDirectory = "Tracker/Tracker New Templates";

TemplatesMenu::TemplatesMenu(const BHandler *target, const char* label, uint32 command)
	:
	BMenu(label),
	fTarget(target),
	fOpenItem(NULL),
	fCommand(command)
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
TemplatesMenu::SetTargetForItems(const BHandler* target)
{
	status_t result = BMenu::SetTargetForItems(target);
	if (fOpenItem)
		fOpenItem->SetTarget(target);

	return result;
}


void
TemplatesMenu::UpdateMenuState()
{
	_BuildMenu(false);
}


bool
TemplatesMenu::_BuildMenu(bool addItems)
{
	// clear everything...
	fOpenItem = nullptr;
	int32 count = CountItems();
	while (count--)
		delete RemoveItem((int32)0);

	// Always create a new message
	BMessage *message = new BMessage(fCommand);
	message->AddString("type","new_folder");
	// add the folder
	IconMenuItem* menuItem = new IconMenuItem(B_TRANSLATE("New folder"),
		message, DIR_FILETYPE, B_MINI_ICON);
	AddItem(menuItem);
	menuItem->SetShortcut('N', 0);

	// the templates folder
	BPath path;
	find_directory (B_USER_SETTINGS_DIRECTORY, &path, true);
	path.Append(kTemplatesDirectory);
	mkdir(path.Path(), 0777);

	count = 0;

	BEntry entry;
	BDirectory templatesDir(path.Path());
	while (templatesDir.GetNextEntry(&entry) == B_OK) {
		BNode node(&entry);
		BNodeInfo nodeInfo(&node);
		char fileName[B_FILE_NAME_LENGTH];
		entry.GetName(fileName);
		if (nodeInfo.InitCheck() == B_OK) {
			char mimeType[B_MIME_TYPE_LENGTH];
			nodeInfo.GetType(mimeType);

			BMimeType mime(mimeType);
			if (mime.IsValid()) {
				if (count == 0)
					AddSeparatorItem();

				count++;

				// If not adding items, we are just seeing if there
				// are any to list.  So if we find one, immediately
				// bail and return the result.
				if (!addItems)
					break;

				entry_ref ref;
				entry.GetRef(&ref);

				BMessage *message = new BMessage(fCommand);
				message->AddString("type","new_file");
				message->AddRef("refs", &ref);
				message->AddString("name", fileName);
				AddItem(new IconMenuItem(fileName, message, &nodeInfo,
					B_MINI_ICON));
			}
		}
	}

	AddSeparatorItem();

	// this is the message sent to open the templates folder
	BMessage *template_message = new BMessage(fCommand);
	template_message->AddString("type","open_template_folder");
	entry_ref dirRef;
	if (templatesDir.GetEntry(&entry) == B_OK)
		entry.GetRef(&dirRef);
	template_message->AddRef("refs", &dirRef);

	// add item to show templates folder
	fOpenItem = new BMenuItem(B_TRANSLATE("Edit templates" B_UTF8_ELLIPSIS),
		template_message);
	AddItem(fOpenItem);
	if (dirRef == entry_ref())
		fOpenItem->SetEnabled(false);

	return count > 0;
}
