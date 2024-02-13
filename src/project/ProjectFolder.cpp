/*
 * Copyright 2023, Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectFolder.h"

#include <Catalog.h>
#include <Directory.h>
#include <Debug.h>
#include <Entry.h>
#include <OutlineListView.h>
#include <Path.h>

#include "ConfigManager.h"
#include "LSPProjectWrapper.h"
#include "LSPServersManager.h"
#include "GenioNamespace.h"
#include "GSettings.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectSettingsWindow"

SourceItem::SourceItem(const BString& path)
	:
	fEntryRef(),
	fType(SourceItemType::FileItem),
	fProjectFolder(nullptr)
{
	status_t status = get_ref_for_path(path.String(), &fEntryRef);
	if (status != B_OK) {
		// TODO: What to do ?
		LogError("Failed to get ref for path %s: %s", path.String(), ::strerror(status));
	}
	BEntry entry(&fEntryRef);
	if (entry.IsDirectory())
		fType = SourceItemType::FolderItem;
	else
		fType = SourceItemType::FileItem;
}


SourceItem::SourceItem(const entry_ref& ref)
	:
	fEntryRef(ref),
	fType(SourceItemType::FileItem),
	fProjectFolder(nullptr)
{
	BEntry entry(&fEntryRef);
	if (entry.IsDirectory())
		fType = SourceItemType::FolderItem;
	else
		fType = SourceItemType::FileItem;
}


SourceItem::~SourceItem()
{
}


const entry_ref*
SourceItem::EntryRef() const
{
	return &fEntryRef;
}


BString	const
SourceItem::Name() const
{
	return fEntryRef.name;
}


void
SourceItem::UpdateEntryRef(const entry_ref& ref)
{
	fEntryRef = ref;
}


// ProjectFolder
ProjectFolder::ProjectFolder(const entry_ref& ref, BMessenger& msgr)
	:
	SourceItem(ref),
	fActive(false),
	fSettings(nullptr),
	fMessenger(msgr),
	fGitRepository(nullptr),
	fIsBuilding(false)
{
	fProjectFolder = this;
	fType = SourceItemType::ProjectFolderItem;

	fFullPath = BPath(EntryRef()).Path();

	try {
		fGitRepository = new GitRepository(fFullPath);
	} catch(const GitException &ex) {
		LogError("Could not create a GitRepository instance on project %s with error %d: %s",
			fFullPath.String(), ex.Error(), ex.what());
	}
}


LSPProjectWrapper*
ProjectFolder::GetLSPServer(const BString& fileType)
{
	for (LSPProjectWrapper* w : fLSPProjectWrappers) {
		if (w->ServerConfig().IsFileTypeSupported(fileType))
			return w;
	}
	LSPProjectWrapper* wrap = LSPServersManager::CreateLSPProject(BPath(fFullPath), fMessenger, fileType);
	if (wrap)
		fLSPProjectWrappers.push_back(wrap);
	return wrap;
}


ProjectFolder::~ProjectFolder()
{
	for (LSPProjectWrapper* w : fLSPProjectWrappers) {
		delete w;
	}
	delete fGitRepository;
	delete fSettings;
}


status_t
ProjectFolder::Open()
{
	ASSERT(fSettings == nullptr);
	fSettings = new ConfigManager(kMsgProjectSettingsUpdated);
	_PrepareSettings();

	status_t status = LoadSettings();
	if (status != B_OK)
		LogInfoF("%s", "Cannot load project settings");

	// not a fatal error, just start with defaults
	return B_OK;
}


status_t
ProjectFolder::Close()
{
	SaveSettings();
	return B_OK;
}


BString const
ProjectFolder::Path() const
{
	return fFullPath;
}


ConfigManager&
ProjectFolder::Settings()
{
	ASSERT(fSettings != nullptr);
	return *fSettings;
}


status_t
ProjectFolder::LoadSettings()
{
	if (fSettings == nullptr)
		return B_NO_INIT;

	BPath path(Path());
	path.Append(GenioNames::kProjectSettingsFile);
	status_t status = fSettings->LoadFromFile({ path.Path(), BPath(Path()) });
	if (status != B_OK) {
		// Try to load old style settings
		status = _LoadOldSettings();
		if (status == B_OK)
			LogTraceF("%s", "Loaded old style settings");
	}

	return status;
}


status_t
ProjectFolder::SaveSettings()
{
	if (fSettings == nullptr)
		return B_NO_INIT;

	BPath path(Path());
	path.Append(GenioNames::kProjectSettingsFile);
	status_t status = fSettings->SaveToFile({ path.Path(), BPath(Path()) });
	if (status != B_OK) {
		LogErrorF("Cannot save settings: %s", ::strerror(status));
	}
	return status;
}


void
ProjectFolder::SetBuildMode(BuildMode mode)
{
	(*fSettings)["build_mode"] = int32(mode);
}


BuildMode
ProjectFolder::GetBuildMode() const
{
	return BuildMode(int32((*fSettings)["build_mode"]));
}


void
ProjectFolder::SetBuildCommand(BString const& command, BuildMode mode)
{
	if (GetBuildMode() == BuildMode::ReleaseMode)
		(*fSettings)["project_release_build_command"] = command;
	else
		(*fSettings)["project_debug_build_command"] = command;
}


BString const
ProjectFolder::GetBuildCommand() const
{
	if (GetBuildMode() == BuildMode::ReleaseMode) {
		BString build = (*fSettings)["project_release_build_command"];
		if (build.IsEmpty())
			build = fGuessedBuildCommand;
		return build;
	} else
		return (*fSettings)["project_debug_build_command"];
}


void
ProjectFolder::SetCleanCommand(BString const& command, BuildMode mode)
{
	if (GetBuildMode() == BuildMode::ReleaseMode)
		(*fSettings)["project_release_clean_command"] = command;
	else
		(*fSettings)["project_debug_clean_command"] = command;
}


BString const
ProjectFolder::GetCleanCommand() const
{
	if (GetBuildMode() == BuildMode::ReleaseMode) {
		BString clean = (*fSettings)["project_release_clean_command"];
		if (clean.IsEmpty())
			clean = fGuessedCleanCommand;
		return clean;
	} else
		return (*fSettings)["project_debug_clean_command"];
}


void
ProjectFolder::SetExecuteArgs(BString const& args, BuildMode mode)
{
	if (GetBuildMode() == BuildMode::ReleaseMode)
		(*fSettings)["project_release_execute_args"] = args;
	else
		(*fSettings)["project_debug_execute_args"] = args;
}


BString const
ProjectFolder::GetExecuteArgs() const
{
	if (GetBuildMode() == BuildMode::ReleaseMode)
		return (*fSettings)["project_release_execute_args"];
	else
		return (*fSettings)["project_debug_execute_args"];
}


void
ProjectFolder::SetTarget(BString const& path, BuildMode mode)
{
	if (GetBuildMode() == BuildMode::ReleaseMode)
		(*fSettings)["project_release_target"] = path;
	else
		(*fSettings)["project_debug_target"] = path;
}


BString const
ProjectFolder::GetTarget() const
{
	if (GetBuildMode() == BuildMode::ReleaseMode)
		return (*fSettings)["project_release_target"];
	else
		return (*fSettings)["project_debug_target"];
}


void
ProjectFolder::SetRunInTerminal(bool enabled)
{
	(*fSettings)["project_run_in_terminal"] = enabled;
}


bool
ProjectFolder::RunInTerminal() const
{
	return (*fSettings)["project_run_in_terminal"];
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


void
ProjectFolder::SetGuessedBuilder(const BString& string)
{
	fGuessedBuildCommand = string;
	fGuessedCleanCommand = string;
	fGuessedCleanCommand.Append(" clean");
}


const rgb_color
ProjectFolder::Color() const
{
	return (*fSettings)["color"];
}


void
ProjectFolder::_PrepareSettings()
{
	ASSERT(fSettings != nullptr);

	GMessage buildModes = {
		{"mode", "options"},
		{"option_1", {
			{"value", (int32)BuildMode::ReleaseMode },
			{"label", "release" }}},
		{"option_2", {
			{"value", (int32)BuildMode::DebugMode },
			{"label", "debug"}}},
	};
	rgb_color color = ui_color(B_PANEL_BACKGROUND_COLOR);
	fSettings->AddConfig("General", "color",
		B_TRANSLATE("Color:"), color, nullptr, kStorageTypeAttribute);

	fSettings->AddConfig("Build", "build_mode",
		B_TRANSLATE("Build mode:"), int32(BuildMode::ReleaseMode), &buildModes);

	fSettings->AddConfig("Build/Release", "project_release_build_command",
		B_TRANSLATE("Build command:"), "");
	fSettings->AddConfig("Build/Release", "project_release_clean_command",
		B_TRANSLATE("Clean command:"), "");
	fSettings->AddConfig("Build/Release", "project_release_execute_args",
		B_TRANSLATE("Execute args:"), "");
	fSettings->AddConfig("Build/Release", "project_release_target",
		B_TRANSLATE("Target:"), "");
	fSettings->AddConfig("Build/Debug", "project_debug_build_command",
		B_TRANSLATE("Build command:"), "");
	fSettings->AddConfig("Build/Debug", "project_debug_clean_command",
		B_TRANSLATE("Clean command:"), "");
	fSettings->AddConfig("Build/Debug", "project_debug_execute_args",
		B_TRANSLATE("Execute args:"), "");
	fSettings->AddConfig("Build/Debug", "project_debug_target",
		B_TRANSLATE("Target:"), "");

	fSettings->AddConfig("Run", "project_run_in_terminal",
		B_TRANSLATE("Run in terminal"), false);
}


status_t
ProjectFolder::_LoadOldSettings()
{
	GSettings oldSettings(Path(), GenioNames::kProjectSettingsFile, 'LOPR');
	status_t status = oldSettings.GetStatus();
	if (status != B_OK)
		return status;

	LogTraceF("%s", "Loading old style settings");

	// Load old style settins into new
	(*fSettings)["build_mode"] = int32(oldSettings.GetInt32("build_mode", BuildMode::ReleaseMode));
	(*fSettings)["project_release_build_command"] = oldSettings.GetString("project_release_build_command", "");
	(*fSettings)["project_debug_build_command"] = oldSettings.GetString("project_debug_build_command", "");
	(*fSettings)["project_release_clean_command"] = oldSettings.GetString("project_release_clean_command", "");
	(*fSettings)["project_debug_clean_command"] = oldSettings.GetString("project_debug_clean_command", "");
	(*fSettings)["project_release_execute_args"] = oldSettings.GetString("project_release_execute_args", "");
	(*fSettings)["project_debug_execute_args"] = oldSettings.GetString("project_debug_execute_args", "");
	(*fSettings)["project_release_target"] = oldSettings.GetString("project_release_target", "");
	(*fSettings)["project_debug_target"] = oldSettings.GetString("project_debug_target", "");
	(*fSettings)["project_run_in_terminal"] = oldSettings.GetBool("project_run_in_terminal", false);

	return B_OK;
}
