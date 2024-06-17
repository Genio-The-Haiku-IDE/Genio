/*
 * Copyright 2024, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6@disroot.org>
 */


#include "ExtensionManager.h"

#include <StringList.h>
#include <filesystem>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include "Utils.h"


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
		BPath path;
		if (entry.GetPath(&path) == B_OK) {
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK) {
				ExtensionInfo extension;
				namespace fs = std::filesystem;
				fs::path stdpath(path.Path());
				extension.Name = stdpath.stem().c_str();
				// check if the extension has a shortcut and ends with "-[shortcut]"
				// BStringList list;
				// BString dash("-");
				// extension.Name.Split(dash, true, list);
				// if (!list.IsEmpty()) {
					// extension.Shortcut = list.Last()[0];
					// remove shortcut
					// dash.Append(extension.Shortcut, 1);
					// extension.Name.RemoveLast(dash);
				// }
				int length = extension.Name.Length();
				if (extension.Name[length-2] == '-') {
					extension.Shortcut = extension.Name[length-1];
					extension.Name.Truncate(length-2);
				} else {
					extension.Shortcut = 0;
				}
				extension.Ref = ref;
				extension.Enabled = true;
				fExtensions.emplace_back(std::move(extension));
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

std::vector<ExtensionInfo>&
ExtensionManager::GetExtensions()
{
	return fExtensions;
}