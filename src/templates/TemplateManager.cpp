/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <Application.h>
#include <AppFileInfo.h>
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
	
	
}

TemplateManager::~TemplateManager()
{
}

status_t
TemplateManager::CopyTemplate(const entry_ref* source, const entry_ref* destination)
{
	return B_OK;
}

status_t
TemplateManager::CreateTemplate(const entry_ref* file)
{
	return B_OK;
}

