/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <Entry.h>
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

class Project;

class SourceItem {
public:
								SourceItem(BString const& path);
								~SourceItem();

	const entry_ref*			EntryRef() const;
	BString	const				Path() const;
	BString	const				Name() const;
	SourceItemType				Type() const { return fType; };

	Project						*GetProject() const { return fProject; }
	void						SetProject(Project* project) { fProject = project; }

	void 						Rename(BString const& path);
private:
	entry_ref					fEntryRef;
protected:
	SourceItemType				fType;
	Project						*fProject;
};


class Project : public SourceItem {
public:
								Project(BString const& path, BMessenger& msgr);
								~Project();

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
