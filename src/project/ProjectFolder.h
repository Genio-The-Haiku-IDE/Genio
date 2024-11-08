/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <Entry.h>
#include <Messenger.h>
#include <ObjectList.h>
#include <String.h>

#include <vector>

#include "GitRepository.h"

using namespace Genio::Git;

class BMessenger;
class ConfigManager;
class LSPProjectWrapper;
class LSPTextDocument;

const uint32 kMsgProjectSettingsUpdated = 'PRJS';

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
					explicit	SourceItem(const BString& path);
					explicit	SourceItem(const entry_ref& ref);
								~SourceItem();

	const entry_ref*			EntryRef() const;
	void						UpdateEntryRef(const entry_ref& ref);

	BString	const				Name() const;
	SourceItemType				Type() const { return fType; };

	ProjectFolder*				GetProjectFolder()	const { return fProjectFolder; }
	void						SetProjectFolder(ProjectFolder *projectFolder)	{ fProjectFolder = projectFolder; }

private:
	entry_ref					fEntryRef;
protected:
	SourceItemType				fType;
	ProjectFolder				*fProjectFolder;
};


class ProjectFolder : public SourceItem {
public:
								ProjectFolder(const entry_ref& ref, BMessenger& msgr);
								~ProjectFolder();

	status_t					Open();
	status_t					Close();

	BString	const				Path() const;

	bool						Active() const { return fActive; }
	void						SetActive(bool status) { fActive = status; }

	ConfigManager&				Settings();
	status_t					LoadSettings();
	status_t					SaveSettings();

	void						SetBuildMode(BuildMode mode);
	BuildMode					GetBuildMode() const;
	bool						IsBuilding() const { return fIsBuilding; }
	void						SetBuildingState(bool isBuilding) { fIsBuilding = isBuilding; }

	void						GuessBuildCommand();

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

	const rgb_color				Color() const;

	LSPProjectWrapper*			GetLSPServer(const BString& fileType);

private:
	void						_PrepareSettings();
	status_t					_LoadOldSettings();

	bool						fActive;
	std::vector<LSPProjectWrapper*>	fLSPProjectWrappers;
	ConfigManager*				fSettings;
	BMessenger					fMessenger;
	GitRepository*				fGitRepository;
	bool						fIsBuilding;
	BString						fFullPath;
};
