/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#ifndef TEMPLATEMANAGER_H
#define TEMPLATEMANAGER_H


#include <Entry.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <SupportDefs.h>

#include <list>
#include <string>


class TemplateManager {

public:

	static status_t		CopyFileTemplate(const entry_ref* source, const entry_ref* destination);
	static status_t		CopyProjectTemplate(const entry_ref* source, const entry_ref* destination,
												const char* name = nullptr);
	static status_t		CreateTemplate(const entry_ref* project);
	static status_t		CreateNewFolder(const entry_ref* destination);
	
	static BString		GetDefaultTemplateDirectory();
	static BString		GetUserTemplateDirectory();

private:

						TemplateManager();
						~TemplateManager();
};


#endif // TEMPLATEMANAGER_H
