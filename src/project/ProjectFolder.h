/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_FOLDER_H
#define PROJECT_FOLDER_H

#include <ObjectList.h>
#include <String.h>
#include <Messenger.h>
#include "GitRepository.h"
#include "GException.h"
#include "GSettings.h"

using namespace Genio::Git;

class LSPProjectWrapper;

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
	bool						RunInTerminal() const;

	auto						GetRepository() { return fGitRepository; }
	void						InitRepository(bool createInitialCommit = true)
									{fGitRepository->Init(createInitialCommit); }

	void						SetGuessedBuilder(const BString& string);

	LSPProjectWrapper*			GetLSPClient() { return fLSPProjectWrapper; }

private:
	bool						fActive;
	BuildMode					fBuildMode;
	BString						fGuessedBuildCommand;
	BString						fGuessedCleanCommand;
	LSPProjectWrapper*			fLSPProjectWrapper;
	GSettings*					fSettings;
	GitRepository*				fGitRepository;
};

#endif // PROJECT_FOLDER_H
