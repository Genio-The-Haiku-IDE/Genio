/*
 * Copyright 2023, Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <OutlineListView.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include <stdexcept>

#include "GSettings.h"
#include "ProjectFolder.h"


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
	// fSettings(fPath, fName, 'LOPR')
{	
	fType = SourceItemType::ProjectFolderItem;
	fProjectFolder = this;
}

ProjectFolder::~ProjectFolder()
{
}

status_t
ProjectFolder::Open()
{
	return B_OK;
}

void
ProjectFolder::SetBuildMode(BuildMode mode)
{
	fBuildMode = mode;
	GSettings fSettings(fPath, fName, 'LOPR');
	fSettings.SetInt32("build_mode", mode);
}

BuildMode
ProjectFolder::GetBuildMode() 
{ 
	GSettings fSettings(fPath, fName, 'LOPR');
	fBuildMode = (BuildMode)fSettings.GetInt32("build_mode", BuildMode::ReleaseMode);
	return fBuildMode;
}

void
ProjectFolder::SetBuildCommand(BString const& command, BuildMode mode)
{
	GSettings fSettings(fPath, fName, 'LOPR');
	if (mode == BuildMode::ReleaseMode)
		fSettings.SetString("project_release_build_command", command);
	else
		fSettings.SetString("project_debug_build_command", command);
}

BString  const
ProjectFolder::GetBuildCommand()
{	
	GSettings fSettings(fPath, fName, 'LOPR');
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings.GetString("project_release_build_command", "");
	else
		return fSettings.GetString("project_debug_build_command", "");
}

void
ProjectFolder::SetCleanCommand(BString const& command, BuildMode mode)
{
	GSettings fSettings(fPath, fName, 'LOPR');
	if (mode == BuildMode::ReleaseMode)
		fSettings.SetString("project_release_clean_command", command);
	else
		fSettings.SetString("project_debug_clean_command", command);
}

BString  const
ProjectFolder::GetCleanCommand()
{
	GSettings fSettings(fPath, fName, 'LOPR');
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings.GetString("project_release_clean_command", "");
	else
		return fSettings.GetString("project_debug_clean_command", "");
}

void
ProjectFolder::SetExecuteArgs(BString const& args, BuildMode mode)
{
	GSettings fSettings(fPath, fName, 'LOPR');
	if (mode == BuildMode::ReleaseMode)
		fSettings.SetString("project_release_execute_args", args);
	else
		fSettings.SetString("project_debug_execute_args", args);
}

BString  const
ProjectFolder::GetExecuteArgs()
{
	GSettings fSettings(fPath, fName, 'LOPR');
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings.GetString("project_release_execute_args", "");
	else
		return fSettings.GetString("project_debug_execute_args", "");
}

void
ProjectFolder::SetTarget(BString const& path, BuildMode mode)
{
	GSettings fSettings(fPath, fName, 'LOPR');
	if (mode == BuildMode::ReleaseMode)
		fSettings.SetString("project_release_target", path);
	else
		fSettings.SetString("project_debug_target", path);
}

BString  const
ProjectFolder::GetTarget()
{
	GSettings fSettings(fPath, fName, 'LOPR');
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings.GetString("project_release_target", "");
	else
		return fSettings.GetString("project_debug_target", "");
}

void
ProjectFolder::RunInTerminal(bool enabled)
{
	GSettings fSettings(fPath, fName, 'LOPR');
	fSettings.SetBool("project_run_in_terminal", enabled);
}

bool
ProjectFolder::RunInTerminal()
{
	GSettings fSettings(fPath, fName, 'LOPR');
	return fSettings.GetBool("project_run_in_terminal", false);
}

void
ProjectFolder::Git(bool enabled)
{
	GSettings fSettings(fPath, fName, 'LOPR');
	fSettings.SetBool("git", enabled);
}

bool
ProjectFolder::Git()
{
	GSettings fSettings(fPath, fName, 'LOPR');
	return fSettings.GetBool("git", false);
}

void
ProjectFolder::ExcludeSettingsOnGit(bool enabled)
{
	GSettings fSettings(fPath, fName, 'LOPR');
	fSettings.SetBool("exclude_settings_git", enabled);
}

bool
ProjectFolder::ExcludeSettingsOnGit()
{
	GSettings fSettings(fPath, fName, 'LOPR');
	return fSettings.GetBool("exclude_settings_git", false);
}