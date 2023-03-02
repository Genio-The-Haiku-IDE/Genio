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
	// fSettings(fPath, fName, 'LOPR')
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
}

status_t
ProjectFolder::Open()
{
	std::string rootURI = "file://" + std::string(fPath.String());
	fLSPClientWrapper->Create(rootURI.c_str());
	return B_OK;
}

void
ProjectFolder::SetBuildMode(BuildMode mode)
{
	fBuildMode = mode;
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	fSettings.SetInt32("build_mode", mode);
}

BuildMode
ProjectFolder::GetBuildMode() 
{ 
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	fBuildMode = (BuildMode)fSettings.GetInt32("build_mode", BuildMode::ReleaseMode);
	return fBuildMode;
}

void
ProjectFolder::SetBuildCommand(BString const& command, BuildMode mode)
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	if (mode == BuildMode::ReleaseMode)
		fSettings.SetString("project_release_build_command", command);
	else
		fSettings.SetString("project_debug_build_command", command);
}

BString  const
ProjectFolder::GetBuildCommand()
{	
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings.GetString("project_release_build_command", "");
	else
		return fSettings.GetString("project_debug_build_command", "");
}

void
ProjectFolder::SetCleanCommand(BString const& command, BuildMode mode)
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	if (mode == BuildMode::ReleaseMode)
		fSettings.SetString("project_release_clean_command", command);
	else
		fSettings.SetString("project_debug_clean_command", command);
}

BString  const
ProjectFolder::GetCleanCommand()
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings.GetString("project_release_clean_command", "");
	else
		return fSettings.GetString("project_debug_clean_command", "");
}

void
ProjectFolder::SetExecuteArgs(BString const& args, BuildMode mode)
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	if (mode == BuildMode::ReleaseMode)
		fSettings.SetString("project_release_execute_args", args);
	else
		fSettings.SetString("project_debug_execute_args", args);
}

BString  const
ProjectFolder::GetExecuteArgs()
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings.GetString("project_release_execute_args", "");
	else
		return fSettings.GetString("project_debug_execute_args", "");
}

void
ProjectFolder::SetTarget(BString const& path, BuildMode mode)
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	if (mode == BuildMode::ReleaseMode)
		fSettings.SetString("project_release_target", path);
	else
		fSettings.SetString("project_debug_target", path);
}

BString  const
ProjectFolder::GetTarget()
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	if (fBuildMode == BuildMode::ReleaseMode)
		return fSettings.GetString("project_release_target", "");
	else
		return fSettings.GetString("project_debug_target", "");
}

void
ProjectFolder::RunInTerminal(bool enabled)
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	fSettings.SetBool("project_run_in_terminal", enabled);
}

bool
ProjectFolder::RunInTerminal()
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	return fSettings.GetBool("project_run_in_terminal", false);
}

void
ProjectFolder::Git(bool enabled)
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	fSettings.SetBool("git", enabled);
}

bool
ProjectFolder::Git()
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	return fSettings.GetBool("git", false);
}

void
ProjectFolder::ExcludeSettingsOnGit(bool enabled)
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	fSettings.SetBool("exclude_settings_git", enabled);
}

bool
ProjectFolder::ExcludeSettingsOnGit()
{
	GSettings fSettings(fPath, GenioNames::kProjectSettingsFile, 'LOPR');
	return fSettings.GetBool("exclude_settings_git", false);
}