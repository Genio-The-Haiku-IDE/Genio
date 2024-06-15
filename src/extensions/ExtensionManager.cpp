/*
 * Copyright 2024, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6@disroot.org>
 */


#include "ExtensionManager.h"

#include <algorithm>

#include <AppFileInfo.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <Resources.h>
#include <Resources.h>
#include <fs_attr.h>

#include "Editor.h"
#include "Utils.h"
#include "Log.h"
#include "wildcard.h"


const char* kExtensionDirectory = "extensions";

// TODO: enrich the scope with element such as Outline, Search Results, and more
const char* kExtensionScopeEditor = "Editor";
const char* kExtensionScopeProject = "Project";
const char* kExtensionScopeProjectItem = "ProjectItem";


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
		BFile file;
		BPath path(&entry);
		if (file.SetTo(&entry, B_READ_ONLY) == B_OK) {
			char attrName[B_ATTR_NAME_LENGTH];
			file.RewindAttrs();

			ExtensionInfo extension;
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK)
				extension.Ref = ref;

			// printf("reading extension %s\n", path.Path());

			bool isValid = false;
			while (file.GetNextAttrName(attrName) == B_OK)
			{
				attr_info info;
				if (file.GetAttrInfo(attrName, &info) == B_OK) {
					// printf("reading attribute %s\n", attrName);
					char *buf = new char[info.size];
					file.ReadAttr(attrName, info.type,0, buf, info.size);

					if (info.type == B_MESSAGE_TYPE) {
						BMessage msg;
						msg.Unflatten(buf);

						extension.ShowInToolsMenu = msg.GetBool("ShowInToolsMenu", false);
						extension.ShowInContextMenu = msg.GetBool("ShowInContextMenu", false);
						auto shortcut = msg.GetString("Shortcut", "");
						if (shortcut == nullptr || strlen(shortcut) > 1)
							extension.Shortcut = '\0';
						else
							extension.Shortcut = shortcut[0];
						extension.Type = msg.GetString("Type", "");

						{
							BString scope;
							int i = 0;
							while(msg.FindString("Scope", i, &scope) == B_OK) {
								extension.Scope.push_back(scope);
								i++;
							}
						}

						{
							BString fileType;
							int i = 0;
							while(msg.FindString("FileTypes", i, &fileType) == B_OK) {
								extension.FileTypes.push_back(fileType);
								i++;
							}
						}

						isValid = true;

						// msg.PrintToStream();

					} else {
						if (strcmp(attrName, "BEOS:APP_VERSION") == 0) {
							uint32 major = static_cast<uint32>(buf[0]);
							uint32 middle = static_cast<uint32>(buf[4]);
							uint32 minor = static_cast<uint32>(buf[8]);
							BString version;
							version.SetToFormat("%d.%d.%d", major, middle, minor);
							extension.Version = version;
							extension.ShortDescription = BString(buf+5*sizeof(uint32), 64);
							extension.LongDescription = BString(buf+5*sizeof(uint32)+64, 256);
							// printf("version %s\n", extension.Version.String());
							// printf("ShortDescription %s\n", extension.ShortDescription.String());
							// printf("LongDescription %s\n", extension.LongDescription.String());
						} else if (strcmp(attrName, "BEOS:APP_SIG") == 0) {
							extension.Signature = buf;
							// printf("Signature %s\n", extension.Signature.String());
						}
					}
					delete[] buf;
				}
			}
			if (isValid)
				fExtensions.push_back(extension);
		} else {
			LogError("Unable to read %s extension\n", path.Path());
		}
	}
	// printf("Extensions loaded %ld\n", fExtensions.size());
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

std::vector<ExtensionInfo>
ExtensionManager::GetScriptExtensions(ExtensionContext& context)
{
	std::vector<ExtensionInfo> extensions;

	for(auto& extension: fExtensions) {
		if (extension.Type == "Script") {
			extension.Enabled = false;
			if ((context.editor != nullptr &&
					std::count(extension.Scope.begin(), extension.Scope.end(), "Editor")) ||
				(context.projectItem != nullptr &&
					std::count(extension.Scope.begin(), extension.Scope.end(), "ProjectBrowser"))) {
				auto filePath = context.editor->FilePath();
				for(auto& ft: extension.FileTypes) {
					if (wildcard(ft.ToLower().String(), filePath.ToLower().String())) {
						extension.Enabled = true;
						break;
					}
				}
			}
			if (context.menuKind == ExtensionToolsMenu && extension.ShowInToolsMenu) {
				extensions.push_back(extension);
			} else if (context.menuKind == ExtensionContextMenu && extension.ShowInContextMenu) {
				if (extension.Enabled)
					extensions.push_back(extension);
			}

		}
	}
	return extensions;
}