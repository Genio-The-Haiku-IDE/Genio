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

								TemplateManager(TemplateManager const&) = delete;
	void 						operator=(TemplateManager const&) = delete;
	
	static status_t				InitCheck() { return instance.fStatus; }
					
	static status_t				CopyTemplate(const entry_ref* source, const entry_ref* destination);
	static status_t				CreateTemplate(const entry_ref* project);
	
	static BString				 GetDefaultTemplateDirectory() { return instance.fDefaultTemplateDirectory; }
	static BString				 GetUserTemplateDirectory() { return instance.fUserTemplateDirectory; }

private:

								TemplateManager();
								~TemplateManager();
	static TemplateManager 		instance;

	BString						fDefaultTemplateDirectory;
	BString						fUserTemplateDirectory;
	status_t					fStatus;
};


#endif // TEMPLATEMANAGER_H
