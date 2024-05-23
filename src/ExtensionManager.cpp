/*
 * Copyright 2023, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6@disroot.org>
 */


#include "ExtensionManager.h"
#include "Utils.h"
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <Resources.h>

const char* kExtensionDirectory = "extensions";

ExtensionManager::ExtensionManager()
{
	_ScanExtensions(_GetSystemDirectory());
	_ScanExtensions(_GetUserDirectory());
}


void
ExtensionManager::_ScanExtensions(BString directory)
{
	BDirectory dir(directory);
	BEntry entry;
	while (dir.GetNextEntry(&entry, true) == B_OK) {
		entry_ref ref;
		if (entry.GetRef(&ref) == B_OK) {
			BResources resources;
			if (resources.SetTo(&ref) == B_OK) {
				size_t size = 0;
				if (resources.HasResource(B_MESSAGE_TYPE, "genio:extension")) {
					auto extInfo = reinterpret_cast<const BMessage*>(resources.LoadResource(
						B_MESSAGE_TYPE,	"genio:extension", &size));
					extInfo->PrintToStream();
				}
			}
		}
	}

}


BString
ExtensionManager::_GetSystemDirectory()
{
	// Default template directory
	BPath templatePath = GetDataDirectory();
	templatePath.Append(kExtensionDirectory);
	return templatePath.Path();
}


BString
ExtensionManager::_GetUserDirectory()
{
	// User template directory
	BPath userPath = GetUserSettingsDirectory();
	userPath.Append(kExtensionDirectory);
	create_directory(userPath.Path(), 0777);

	return userPath.Path();
}