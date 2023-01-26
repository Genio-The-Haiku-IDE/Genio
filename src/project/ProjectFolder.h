/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_FOLDER_H
#define PROJECT_FOLDER_H

#include <ObjectList.h>
#include <String.h>

enum GENIO_SETTINGS_KEY {
	RELEASE_BUILD_COMMAND,
	DEBUG_BUILD_COMMAND,
	RELEASE_CLEAN_COMMAND,
	DEBUG_CLEAN_COMMAND,
	RELEASE_BUILD_MODE,
	DEBUG_BUILD_MODE,
	RELEASE_TARGET_PATH,
	DEBUG_TARGET_PATH ,
	RUN_IN_TERMINAL,
	SOURCE_CONTROL_ACTIVE
};

enum SourceItemType {
	FileItem,
	FolderItem,
	ProjectFolderItem
};

class ProjectFolder;

class SourceItem {
public:
								SourceItem(BString const& path);
								~SourceItem();
								
	BString	const				Path() { return fPath; }
	BString	const				Name() { return fName; };
	SourceItemType				Type() { return fType; };
	
	bool						Active() const { return fActive; }
	void						Active(bool status) { fActive = status; }
	
	ProjectFolder				*GetProjectFolder()	{ return fProjectFolder; }
								
protected:
	BString						fPath;
	BString						fName;
	SourceItemType				fType;
	bool						fActive;
	ProjectFolder				*fProjectFolder;
};

class ProjectFolder : public SourceItem {
public:
								ProjectFolder(BString const& path);
								~ProjectFolder();
	
	status_t					Open();
	status_t					Close();
	
	bool						HasConfig() const { return fHasConfig; }
	void						InitConfig();
	void						ResetConfig();

private:

	bool						fHasConfig;

};

#endif // PROJECT_FOLDER_H
