/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_FOLDER_H
#define PROJECT_FOLDER_H


//#include <Messenger.h>
#include <ObjectList.h>
#include <String.h>

#include "GitRepository.h"

using namespace Genio::Git;

class BMessenger;
class LSPProjectWrapper;
class GSettings;

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

	BString	const				Path() const { return fPath; }
	BString	const				Name() const { return fName; };
	SourceItemType				Type() const { return fType; };

	ProjectFolder				*GetProjectFolder()	const { return fProjectFolder; }
	void						SetProjectFolder(ProjectFolder *projectFolder)	{ fProjectFolder = projectFolder; }

	void 						Rename(BString const& path);

protected:
	BString						fPath;
	BString						fName;
	SourceItemType				fType;
	ProjectFolder				*fProjectFolder;
};


class ProjectFolder : public SourceItem {
public:
								ProjectFolder(BString const& path, BMessenger& msgr);
								~ProjectFolder();

	status_t					Open();
	status_t					Close();

	void						LoadDefaultSettings();
	void						SaveSettings();

	bool						Active() const { return fActive; }
	void						SetActive(bool status) { fActive = status; }

	void						SetBuildMode(BuildMode mode);
	BuildMode					GetBuildMode();
	bool						IsBuilding() const { return fIsBuilding; }
	void						SetBuildingState(bool isBuilding) { fIsBuilding = isBuilding; }

	void						SetCleanCommand(BString const& command, BuildMode mode);
	BString const				GetCleanCommand() const;

	void						SetBuildCommand(BString const& command, BuildMode mode);
	BString const				GetBuildCommand() const;

	void						SetExecuteArgs(BString const& args, BuildMode mode);
	BString const				GetExecuteArgs() const;

	void						SetTarget(BString const& path, BuildMode mode);
	BString const				GetTarget() const;

	void						SetRunInTerminal(bool enabled);
	bool						RunInTerminal() const;

	GitRepository*				GetRepository() const;
	void						InitRepository(bool createInitialCommit = true);

	void						SetGuessedBuilder(const BString& string);

	LSPProjectWrapper*			GetLSPClient() const;

private:
	bool						fActive;
	BuildMode					fBuildMode;
	BString						fGuessedBuildCommand;
	BString						fGuessedCleanCommand;
	LSPProjectWrapper*			fLSPProjectWrapper;
	GSettings*					fSettings;
	GitRepository*				fGitRepository;
	bool						fIsBuilding;
};

#endif // PROJECT_FOLDER_H
