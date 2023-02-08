/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_FOLDER_H
#define PROJECT_FOLDER_H

#include <ObjectList.h>
#include <String.h>

#include "GSettings.h"

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

enum BuildMode {
	ReleaseMode,
	DebugMode
};

class ProjectFolder;

class SourceItem {
public:
								SourceItem(BString const& path);
								~SourceItem();
								
	BString	const				Path() { return fPath; }
	BString	const				Name() { return fName; };
	SourceItemType				Type() { return fType; };
	
	ProjectFolder				*GetProjectFolder()	{ return fProjectFolder; }
	void						SetProjectFolder(ProjectFolder *projectFolder)	{ fProjectFolder = projectFolder; }
								
protected:
	BString						fPath;
	BString						fName;
	SourceItemType				fType;
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

	bool						Active() const { return fActive; }
	void						Active(bool status) { fActive = status; }	
	
	void						SetBuildMode(BuildMode mode);
	BuildMode					GetBuildMode();
	
	void						SetCleanCommand(BString const& command, BuildMode mode);
	BString const				GetCleanCommand();
	
	void						SetBuildCommand(BString const& command, BuildMode mode);
	BString const				GetBuildCommand();
	
	void						SetExecuteArgs(BString const& args, BuildMode mode);
	BString const				GetExecuteArgs();
	
	void						SetTarget(BString const& path, BuildMode mode);
	BString const				GetTarget();
	
	void						RunInTerminal(bool enabled);
	bool						RunInTerminal();
	
	void						Git(bool enabled);
	bool						Git();
	
	void						ExcludeSettingsOnGit(bool enabled);
	bool						ExcludeSettingsOnGit();

private:

	bool						fHasConfig;
	bool						fActive;
	bool						fRunInTerminal;
	bool						fGitEnabled;
	BuildMode					fBuildMode;
	BString						fTarget;
	BString						fBuildCommand;
	
	// GSettings					fSettings;
};

#endif // PROJECT_FOLDER_H
