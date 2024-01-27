/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef TEMPLATEMANAGER_H
#define TEMPLATEMANAGER_H


#include <Entry.h>
#include <String.h>

class TemplateManager {

public:

	static status_t		CopyFileTemplate(const entry_ref* source, const entry_ref* destination, entry_ref* newFileRef);
	static status_t		CopyProjectTemplate(const entry_ref* source, const entry_ref* destination,
												const char* name = nullptr);
	static status_t		CreateTemplate(const entry_ref* project);
	static status_t		CreateNewFolder(const entry_ref* destination, entry_ref* newFolderRef);

	static BString		GetDefaultTemplateDirectory();
	static BString		GetUserTemplateDirectory();

private:

						TemplateManager();
						~TemplateManager();
};


#endif // TEMPLATEMANAGER_H
