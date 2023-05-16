/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <Application.h>
#include <AppFileInfo.h>
<<<<<<< HEAD
#include <Catalog.h>
#include <CopyEngine.h>
#include <EntryOperationEngineBase.h>
#include <Entry.h>
#include <Roster.h>

#include "FSUtils.h"
#include "GenioNamespace.h"
#include "Log.h"
#include "TemplateManager.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TemplateManager"


const char* kTemplateDirectory = "templates";

using Entry = BPrivate::BEntryOperationEngineBase::Entry;

TemplateManager::TemplateManager()
{
=======
#include <Roster.h>

#include "GenioNamespace.h"
#include "TemplateManager.h"

TemplateManager TemplateManager::instance;
const char* kTemplateDirectory = "templates";

TemplateManager::TemplateManager()
{
	// Default template directory
	app_info info;
	fStatus = be_app->GetAppInfo(&info);
	if (fStatus == B_OK) {
		BPath genioPath(&info.ref);
		genioPath.Append(kTemplateDirectory);
		mkdir(genioPath.Path(), 0777);
		instance.fDefaultTemplateDirectory = genioPath.Path();
	}

	// User template directory
	BPath userPath;
	find_directory (B_USER_SETTINGS_DIRECTORY, &userPath, true);
	userPath.Append(GenioNames::kApplicationName);
	userPath.Append(kTemplateDirectory);
	mkdir(userPath.Path(), 0777);
	instance.fUserTemplateDirectory = userPath.Path();
	
	
>>>>>>> 76497bb (Initial work on a brand new "File->New" dynamic menu based on templates files and directories)
}

TemplateManager::~TemplateManager()
{
}

status_t
<<<<<<< HEAD
TemplateManager::CopyFileTemplate(const entry_ref* source, const entry_ref* destination)
{	
	status_t status = B_NOT_INITIALIZED;
	
	// Copy template file to destination
	BPath sourcePath(source);
	Entry sourceEntry(sourcePath.Path());
	
	BPath destPath(destination);
	destPath.Append(source->name);
	Entry destEntry(destPath.Path());
	
	status = BCopyEngine().CopyEntry(sourceEntry, destEntry);
	if (status != B_OK) {
		BString err(strerror(status));
		LogError("Error creating new file %s in %s: %s", sourcePath.Path(), destPath.Path(),err.String());
	}

	chmod(destPath.Path(), 0777);
	
	return status;
}

status_t
TemplateManager::CopyProjectTemplate(const entry_ref* source, const entry_ref* destination, 
										const char* name)
{	
	status_t status = B_NOT_INITIALIZED;
	
	BPath sourcePath(source);
	Entry sourceEntry(sourcePath.Path());
	
	BPath destPath(destination);
	destPath.Append(name);
	Entry destEntry(destPath.Path());
	
	status = BCopyEngine(BCopyEngine::COPY_RECURSIVELY).CopyEntry(sourceEntry, destEntry);
	if (status != B_OK) {
		BString err(strerror(status));
		LogError("Error creating new template %s in %s: %s", sourcePath.Path(), destPath.Path(),err.String());
	}
	
	// TODO: Default templates in a tipical HPKG installation are readonly
	// we are setting all permissions recursively here
	chmodr(destPath.Path(), fs::perms::all);
	
	
	return status;
=======
TemplateManager::CopyTemplate(const entry_ref* source, const entry_ref* destination)
{
	return B_OK;
>>>>>>> 76497bb (Initial work on a brand new "File->New" dynamic menu based on templates files and directories)
}

status_t
TemplateManager::CreateTemplate(const entry_ref* file)
{
	return B_OK;
}

<<<<<<< HEAD
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
	app_info info;
	BPath templatePath;
	if (be_app->GetAppInfo(&info) == B_OK) {
		// This code should work both for the case where Genio is
		// in the "app" subdirectory, like in the repo,
		// and when it's in the package.
		BPath genioPath(&info.ref);
		BPath parentPath;
		if (genioPath.GetParent(&parentPath) == B_OK) {
			templatePath = parentPath;
			templatePath.Append("data");
			templatePath.Append(kTemplateDirectory);
			// Genio
			// data/templates/
			if (!BEntry(templatePath.Path()).IsDirectory()) {
				// app/Genio
				// data/templates/
				parentPath.GetParent(&templatePath);
				templatePath.Append("data");
				templatePath.Append(kTemplateDirectory);
			}
		}
	}
	return templatePath.Path();
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

=======
>>>>>>> 76497bb (Initial work on a brand new "File->New" dynamic menu based on templates files and directories)
