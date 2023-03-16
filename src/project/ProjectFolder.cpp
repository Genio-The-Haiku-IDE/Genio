/*
 * Copyright 2023, Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <OutlineListView.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include <stdexcept>

#include "GenioNamespace.h"
#include "GSettings.h"
#include "ProjectFolder.h"
#include "LSPClientWrapper.h"

SourceItem::SourceItem(BString const& path) 
{
	fPath = path;
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

ProjectFolder::ProjectFolder(BString const& path)
	: 
	SourceItem(path)
{	
	fType = SourceItemType::ProjectFolderItem;
	fProjectFolder = this;
	fActive = false;
	fLSPClientWrapper = new LSPClientWrapper();
}

ProjectFolder::~ProjectFolder()
{
	fLSPClientWrapper->Dispose();
	delete fLSPClientWrapper;

	delete fSettings;
}

status_t
ProjectFolder::Open()
{
	std::string rootURI = "file://" + std::string(fPath.String());
	fLSPClientWrapper->Create(rootURI.c_str());
	
	fSettings = new GSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	
	return B_OK;
}

status_t
ProjectFolder::Close()
{
	status_t status;
	
	fSettings->Save();
	status = fSettings->GetStatus();
	
	return status;
}

void						
ProjectFolder::LoadDefaultSettings() {
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
ProjectFolder::SaveSettings() {
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

BString  const
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

BString  const
ProjectFolder::GetCleanCommand()
{
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings->GetString("project_release_clean_command", "");
	else
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

BString  const
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

BString  const
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