/*
 * Copyright 2022 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef WORKSPACE_PROJECTFOLDER_H
#define WORKSPACE_PROJECTFOLDER_H

#include <ObjectList.h>

#include "GenioCommon.h"
#include "GenioNamespace.h"
#include "TPreferences.h"


enum GENIO_SETTINGS_KEY {
	RELEASE_BUILD_COMMAND,
	DEBU_BUILD_COMMAND,
	RELEASE_CLEAN_COMMAND,
	DEBUG_CLEAN_COMMAND,
	RELEASE_BUILD_MODE,
	DEBUG_BUILD_MODE,
	RELEASE_TARGET_PATH,
	DEBUG_TARGET_PATH ,
	RUN_IN_TERMINAL,
	SOURCE_CONTROL_ACTIVE
};

class ProjectFolder;

class SourceFile {
public:
								SourceFile(const char *path);
								~SourceFile();
								
	BString						GetPath();
	ProjectFolder				*GetProjectFolder();
								
private:
	BString						fPath;
	ProjectFolder				*fProjectFolder;
};

class ProjectFolder {
public:
								ProjectFolder(BString const& name);
								~ProjectFolder();
	
	status_t					Open(bool activate);
	status_t					Close();

	BString	const				Name() const { return fName; }
	BString						BasePath() const { return fProjectDirectory; }
	
	BObjectList<SourceFile> 	GetItems();	
	
	template <class T>
	T							GetConfig(GENIO_SETTINGS_KEY key);
	template <class T>
	void						SetConfig(GENIO_SETTINGS_KEY key, T value);

private:

	BString						fName;
	BString						fProjectDirectory;
	BObjectList<SourceFile>		fFolderContent;
	
	SourceFile*					_BrowseFolder(BEntry *entry, SourceFile *file);

};

#endif // WORKSPACE_PROJECTFOLDER_H
