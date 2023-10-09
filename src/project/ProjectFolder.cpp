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

#include <stdexcept>

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


ProjectFolder::ProjectFolder(BString const& path)
	: 
	SourceItem(path),
	fActive(false),
	fBuildMode(BuildMode::ReleaseMode),
	fLSPProjectWrapper(nullptr),
	fSettings(nullptr)
{
	fProjectFolder = this;
	fType = SourceItemType::ProjectFolderItem;
	
	fLSPProjectWrapper = new LSPProjectWrapper(fPath.String());
}


ProjectFolder::~ProjectFolder()
{
	if (fLSPProjectWrapper != nullptr) {
		fLSPProjectWrapper->Dispose();
//		delete fLSPClientWrapper; //TODO: BLooper autodeletes itself?
	}
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
	fSettings->SetBool("git", false);
	fSettings->SetBool("exclude_settings_git", false);
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
ProjectFolder::GetBuildMode() 
{ 
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
ProjectFolder::GetBuildCommand()
{	
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings->GetString("project_release_build_command", "");
	else
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
ProjectFolder::GetCleanCommand()
{
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings->GetString("project_release_clean_command", "");
	else
		return fSettings->GetString("project_debug_clean_command", "");
}


void
ProjectFolder::SetBuildOnSave(bool enabled)
{
	fSettings->SetBool("build_on_save", enabled);
}


bool
ProjectFolder::SaveOnBuild() const
{
	return fSettings->GetBool("save_on_build", false);
}


void
ProjectFolder::SetSaveOnBuild(bool enabled)
{
	fSettings->SetBool("save_on_build", enabled);
}


bool
ProjectFolder::BuildOnSave() const
{
	return fSettings->GetBool("build_on_save", false);
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
ProjectFolder::GetExecuteArgs()
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
ProjectFolder::GetTarget()
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
ProjectFolder::RunInTerminal()
{
	return fSettings->GetBool("project_run_in_terminal", false);
}


void
ProjectFolder::Git(bool enabled)
{
	fSettings->SetBool("git", enabled);
}


bool
ProjectFolder::Git()
{
	return fSettings->GetBool("git", false);
}


void
ProjectFolder::ExcludeSettingsOnGit(bool enabled)
{
	fSettings->SetBool("exclude_settings_git", enabled);
}


bool
ProjectFolder::ExcludeSettingsOnGit()
{
	return fSettings->GetBool("exclude_settings_git", false);
}
