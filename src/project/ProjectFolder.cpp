/*
 * Copyright 2023, Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectFolder.h"

#include <Directory.h>
#include <Debug.h>
#include <Entry.h>
#include <OutlineListView.h>
#include <Path.h>

#include "LSPProjectWrapper.h"
#include "GenioNamespace.h"
#include "GSettings.h"

SourceItem::SourceItem(BString const& path)
	:
	fPath(path),
	fName(),
	fType(SourceItemType::FileItem),
	fProjectFolder(nullptr)
{
	BPath _path(path);
	fName = _path.Leaf();

	BEntry entry(path);
	if (entry.IsDirectory())
		fType = SourceItemType::FolderItem;
	else
		fType = SourceItemType::FileItem;
}


SourceItem::~SourceItem()
{
}


void
SourceItem::Rename(BString const& path)
{
	fPath = path;
	BPath _path(path);
	fName = _path.Leaf();
}


ProjectFolder::ProjectFolder(BString const& path, BMessenger& msgr)
	:
	SourceItem(path),
	fActive(false),
	fBuildMode(BuildMode::ReleaseMode),
	fLSPProjectWrapper(nullptr),
	fSettings(nullptr),
	fGitRepository(nullptr),
	fIsBuilding(false)
{
	fProjectFolder = this;
	fType = SourceItemType::ProjectFolderItem;

	try {
		fGitRepository = new GitRepository(path);
	} catch(const GitException &ex) {
		LogError("Could not create a GitRepository instance on project %s with error %d: %s",
			path.String(), ex.Error(), ex.what());
	}

	fLSPProjectWrapper = new LSPProjectWrapper(fPath.String(), msgr);
}


ProjectFolder::~ProjectFolder()
{
	if (fLSPProjectWrapper != nullptr) {
		fLSPProjectWrapper->Dispose();
		delete fLSPProjectWrapper;
	}
	delete fGitRepository;
	delete fSettings;
}


status_t
ProjectFolder::Open()
{
	fSettings = new GSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	return B_OK;
}


status_t
ProjectFolder::Close()
{
	status_t status = fSettings->GetStatus();
	return status;
}


void
ProjectFolder::LoadDefaultSettings()
{
	ASSERT(fSettings != nullptr);

	fSettings->MakeEmpty();
	fSettings->SetInt32("build_mode", BuildMode::ReleaseMode);
	fSettings->SetString("project_release_build_command", "");
	fSettings->SetString("project_debug_build_command", "");
	fSettings->SetString("project_release_clean_command", "");
	fSettings->SetString("project_debug_clean_command", "");
	fSettings->SetString("project_release_execute_args", "");
	fSettings->SetString("project_debug_execute_args", "");
	fSettings->SetString("project_release_target", "");
	fSettings->SetString("project_debug_target", "");
	fSettings->SetBool("project_run_in_terminal", false);
}


void
ProjectFolder::SaveSettings()
{
	fSettings->Save();
}


void
ProjectFolder::SetBuildMode(BuildMode mode)
{
	fBuildMode = mode;
	fSettings->SetInt32("build_mode", mode);
}


BuildMode
ProjectFolder::GetBuildMode() /* const */
{
	// TODO: Why are we SETting the mode here ?
	fBuildMode = (BuildMode)fSettings->GetInt32("build_mode", BuildMode::ReleaseMode);
	return fBuildMode;
}


void
ProjectFolder::SetBuildCommand(BString const& command, BuildMode mode)
{
	if (mode == BuildMode::ReleaseMode)
		fSettings->SetString("project_release_build_command", command);
	else
		fSettings->SetString("project_debug_build_command", command);
}


BString const
ProjectFolder::GetBuildCommand() const
{
	if (fBuildMode == BuildMode::ReleaseMode) {
		BString build = fSettings->GetString("project_release_build_command", "");
		if (build == "")
			build = fGuessedBuildCommand;
		return build;
	} else
		return fSettings->GetString("project_debug_build_command", "");
}


void
ProjectFolder::SetCleanCommand(BString const& command, BuildMode mode)
{
	if (mode == BuildMode::ReleaseMode)
		fSettings->SetString("project_release_clean_command", command);
	else
		fSettings->SetString("project_debug_clean_command", command);
}


BString const
ProjectFolder::GetCleanCommand() const
{
	if (fBuildMode == BuildMode::ReleaseMode) {
		BString clean = fSettings->GetString("project_release_clean_command", "");
		if (clean == "")
			clean = fGuessedCleanCommand;
		return clean;
	} else
		return fSettings->GetString("project_debug_clean_command", "");
}


void
ProjectFolder::SetExecuteArgs(BString const& args, BuildMode mode)
{
	if (mode == BuildMode::ReleaseMode)
		fSettings->SetString("project_release_execute_args", args);
	else
		fSettings->SetString("project_debug_execute_args", args);
}


BString const
ProjectFolder::GetExecuteArgs() const
{
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings->GetString("project_release_execute_args", "");
	else
		return fSettings->GetString("project_debug_execute_args", "");
}


void
ProjectFolder::SetTarget(BString const& path, BuildMode mode)
{
	if (mode == BuildMode::ReleaseMode)
		fSettings->SetString("project_release_target", path);
	else
		fSettings->SetString("project_debug_target", path);
}


BString const
ProjectFolder::GetTarget() const
{
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings->GetString("project_release_target", "");
	else
		return fSettings->GetString("project_debug_target", "");
}


void
ProjectFolder::RunInTerminal(bool enabled)
{
	fSettings->SetBool("project_run_in_terminal", enabled);
}


bool
ProjectFolder::RunInTerminal() const
{
	return fSettings->GetBool("project_run_in_terminal", false);
}


GitRepository*
ProjectFolder::GetRepository() const
{
	return fGitRepository;
}


void
ProjectFolder::InitRepository(bool createInitialCommit)
{
	fGitRepository->Init(createInitialCommit);
}


LSPProjectWrapper*
ProjectFolder::GetLSPClient() const
{
	return fLSPProjectWrapper;
}


void
ProjectFolder::SetGuessedBuilder(const BString& string)
{
	fGuessedBuildCommand = string;
	fGuessedCleanCommand = string;
	fGuessedCleanCommand.Append(" clean");
}