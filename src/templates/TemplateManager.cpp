/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <Application.h>
#include <AppFileInfo.h>
#include <Catalog.h>
#include <Entry.h>
#include <Roster.h>

#include "FSUtils.h"
#include "GenioNamespace.h"
#include "Log.h"
#include "TemplateManager.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TemplateManager"

const char* kTemplateDirectory = "templates";

TemplateManager::TemplateManager()
{
}

TemplateManager::~TemplateManager()
{
}

status_t
TemplateManager::CopyFileTemplate(const entry_ref* source, const entry_ref* destination)
{	
	status_t status = B_NOT_INITIALIZED;
	
	// Copy template file to destination
	BEntry sourceEntry(source);
	BEntry destEntry(destination);
	BPath destPath;
	
	destEntry.GetPath(&destPath);
	destPath.Append(source->name, true);
	destEntry.SetTo(destPath.Path());
	status = CopyFile(&sourceEntry, &destEntry, false);
	if (status != B_OK) {
		LogError("Error creating new file %s in %s", source->name, destination->name);
	}
	
	return status;
}

status_t
TemplateManager::CopyProjectTemplate(const entry_ref* source, const entry_ref* destination, 
										const char* name)
{	
	status_t status = B_NOT_INITIALIZED;
	
	return status;
}

status_t
TemplateManager::CreateTemplate(const entry_ref* file)
{
	return B_OK;
}

status_t
TemplateManager::CreateNewFolder(const entry_ref* destination)
{
	status_t status = B_NOT_INITIALIZED;
	BDirectory dir(destination);
	status = dir.CreateDirectory(B_TRANSLATE("New folder"), nullptr);
	if (status != B_OK) {
		LogError("Invalid destination directory [%s]", destination->name);
	}
	return status;
}

BString
TemplateManager::GetDefaultTemplateDirectory()
{
	// Default template directory
	BString retString;
	app_info info;
	
	if (be_app->GetAppInfo(&info) == B_OK) {
		BPath genioPath(&info.ref);
		BPath parent;
		genioPath.GetParent(&parent);
		parent.Append(kTemplateDirectory);
		retString = parent.Path();
	}
	return retString;
}

BString
TemplateManager::GetUserTemplateDirectory()
{
	// User template directory
	BString retString;
	BPath userPath;
	
	status_t status = find_directory (B_USER_SETTINGS_DIRECTORY, &userPath, true);
	if (status == B_OK) {
		userPath.Append(GenioNames::kApplicationName);
		userPath.Append(kTemplateDirectory);
		mkdir(userPath.Path(), 0777);
		retString = userPath.Path();	
	}
	return retString;
}

