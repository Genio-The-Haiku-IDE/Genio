/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GenioNamespace.h"

#include <Application.h>
#include <AppFileInfo.h>
#include <Roster.h>
#include <StringList.h>

#include "DefaultSettingsKeys.h"
#include "TPreferences.h"


namespace GenioNames
{

int32
CompareVersion(const BString appVersion, const BString fileVersion)
{
	BStringList appVersionList, fileVersionList;

	if (appVersion == fileVersion)
		return 0;

	appVersion.Split(".", true, appVersionList);
	fileVersion.Split(".", true, fileVersionList);

	// Shoud not happen, but return just in case
	if (appVersionList.CountStrings() != fileVersionList.CountStrings())
		// TODO notify
		return 0;

	for (int32 index = 0; index < appVersionList.CountStrings(); index++) {
		if (appVersionList.StringAt(index) == fileVersionList.StringAt(index))
			continue;
		if (appVersionList.StringAt(index) < fileVersionList.StringAt(index))
			return -1;
		else if (appVersionList.StringAt(index) > fileVersionList.StringAt(index))
			return 1;
	}
	// Should not get here
	return 0;
}

BString
GetSignature()
{
	char signature[B_MIME_TYPE_LENGTH];
	app_info appInfo;

	if (be_app->GetAppInfo(&appInfo) == B_OK) {
		BFile file(&appInfo.ref, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BAppFileInfo appFileInfo(&file);
			if (appFileInfo.InitCheck() == B_OK) {
				appFileInfo.GetSignature(signature);
			}
		}
	}

	return signature;
}

/*
 * retcode: IsEmpty(), "0.0.0.0", version
 */
BString
GetVersionInfo()
{
	BString version("");
	version_info info = {0};
	app_info appInfo;

	if (be_app->GetAppInfo(&appInfo) == B_OK) {
		BFile file(&appInfo.ref, B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			BAppFileInfo appFileInfo(&file);
			if (appFileInfo.InitCheck() == B_OK) {
				appFileInfo.GetVersionInfo(&info, B_APP_VERSION_KIND);
				version << info.major << "." << info.middle
					<< "." << info.minor << "." << info.internal;
			}
		}
	}

	return version;
}

SettingsVars Settings;

status_t
SaveSettingsVars()
{
	status_t status;
	TPreferences file(kSettingsFileName, kApplicationName, 'IDSE');

	if ((status = file.InitCheck()) != B_OK)
		return status;
		
	status += file.SetString("projects_directory",	Settings.projects_directory);
	status += file.SetInt32("fullpath_title", Settings.fullpath_title);
	status += file.SetInt32("reopen_projects", Settings.reopen_projects);
	status += file.SetInt32("reopen_files", Settings.reopen_files);
	status += file.SetInt32("show_projects", Settings.show_projects);
	status += file.SetInt32("show_output", Settings.show_output);
	status += file.SetInt32("show_toolbar", Settings.show_toolbar);
	status += file.SetInt32("edit_fontsize", Settings.edit_fontsize);
	status += file.SetInt32("syntax_highlight", Settings.syntax_highlight);
	status += file.SetInt32("tab_width", Settings.tab_width);
	status += file.SetInt32("brace_match", Settings.brace_match);
	status += file.SetInt32("save_caret", Settings.save_caret);
	status += file.SetInt32("show_linenumber", Settings.show_linenumber);
	status += file.SetInt32("show_commentmargin", Settings.show_commentmargin);
	status += file.SetInt32("mark_caretline", Settings.mark_caretline);
	status += file.SetInt32("show_edgeline", Settings.show_edgeline);
	status += file.SetString("edgeline_column", Settings.edgeline_column);
	status += file.SetInt32("enable_folding", Settings.enable_folding);
	status += file.SetInt32("enable_notifications", Settings.enable_notifications);
	status += file.SetInt32("wrap_console", Settings.wrap_console);
	status += file.SetInt32("console_banner", Settings.console_banner);
	status += file.SetInt32("editor_zoom", Settings.editor_zoom);
	status += file.SetBool("find_wrap", 	  Settings.find_wrap);
	status += file.SetBool("find_whole_word", Settings.find_whole_word);
	status += file.SetBool("find_match_case", Settings.find_match_case);
	status += file.SetInt32("log_destination", Settings.log_destination);
	
	return status;
}
		
status_t
LoadSettingsVars()
{
	status_t status;
	TPreferences file(kSettingsFileName, kApplicationName, 'IDSE');

	if ((status = file.InitCheck()) != B_OK)
		return status;

	status += file.FindString("projects_directory",	&Settings.projects_directory);
	status += file.FindInt32("fullpath_title", &Settings.fullpath_title);
	status += file.FindInt32("reopen_projects", &Settings.reopen_projects);
	status += file.FindInt32("reopen_files", &Settings.reopen_files);
	status += file.FindInt32("show_projects", &Settings.show_projects);
	status += file.FindInt32("show_output", &Settings.show_output);
	status += file.FindInt32("show_toolbar", &Settings.show_toolbar);
	status += file.FindInt32("edit_fontsize", &Settings.edit_fontsize);
	status += file.FindInt32("syntax_highlight", &Settings.syntax_highlight);
	status += file.FindInt32("tab_width", &Settings.tab_width);
	status += file.FindInt32("brace_match", &Settings.brace_match);
	status += file.FindInt32("save_caret", &Settings.save_caret);
	status += file.FindInt32("show_linenumber", &Settings.show_linenumber);
	status += file.FindInt32("show_commentmargin", &Settings.show_commentmargin);
	status += file.FindInt32("mark_caretline", &Settings.mark_caretline);
	status += file.FindInt32("show_edgeline", &Settings.show_edgeline);
	status += file.FindString("edgeline_column", &Settings.edgeline_column);
	status += file.FindInt32("enable_folding", &Settings.enable_folding);
	status += file.FindInt32("enable_notifications", &Settings.enable_notifications);
	status += file.FindInt32("wrap_console", &Settings.wrap_console);
	status += file.FindInt32("console_banner", &Settings.console_banner);
	status += file.FindInt32("editor_zoom", &Settings.editor_zoom);
	status += file.FindBool("find_wrap", 	   &Settings.find_wrap);
	status += file.FindBool("find_whole_word", &Settings.find_whole_word);
	status += file.FindBool("find_match_case", &Settings.find_match_case);
	status += file.FindInt32("log_destination", &Settings.log_destination);

	return status;
}

/*
 * When new app version is created,
 */
status_t
UpdateSettingsFile()
{
	status_t status;
	BString stringVal;
	int32	intVal;
	bool	boolVal;

	TPreferences settings(kSettingsFileName, kApplicationName, 'IDSE');

	if ((status = settings.InitCheck()) != B_OK)
		return status;

	// General Page
	if (settings.FindString("projects_directory", &stringVal) != B_OK)
		settings.SetBString("projects_directory", kSKProjectsDirectory);
	if (settings.FindInt32("fullpath_title", &intVal) != B_OK)
		settings.SetInt32("fullpath_title", kSKFullPathTitle);
	// General Startup Page
	if (settings.FindInt32("reopen_projects", &intVal) != B_OK)
		settings.SetInt32("reopen_projects", kSKReopenProjects);
	if (settings.FindInt32("reopen_files", &intVal) != B_OK)
		settings.SetInt32("reopen_files", kSKReopenFiles);
	if (settings.FindInt32("show_projects", &intVal) != B_OK)
		settings.SetInt32("show_projects", kSKShowProjects);
	if (settings.FindInt32("show_output", &intVal) != B_OK)
		settings.SetInt32("show_output", kSKShowOutput);
	if (settings.FindInt32("show_toolbar", &intVal) != B_OK)
		settings.SetInt32("show_toolbar", kSKShowToolBar);
	// Editor Page
	if (settings.FindInt32("edit_fontsize", &intVal) != B_OK)
		settings.SetInt32("edit_fontsize", kSKEditorFontSize);
	if (settings.FindInt32("syntax_highlight", &intVal) != B_OK)
		settings.SetInt32("syntax_highlight", kSKSyntaxHighlight);
	if (settings.FindInt32("tab_width", &intVal) != B_OK)
		settings.SetInt32("tab_width", kSKTabWidth);
	if (settings.FindInt32("brace_match", &intVal) != B_OK)
		settings.SetInt32("brace_match", kSKBraceMatch);
	if (settings.FindInt32("save_caret", &intVal) != B_OK)
		settings.SetInt32("save_caret", kSKSaveCaret);
	// Editor Visual Page
	if (settings.FindInt32("show_linenumber", &intVal) != B_OK)
		settings.SetInt32("show_linenumber", kSKShowLineNumber);
	if (settings.FindInt32("show_commentmargin", &intVal) != B_OK)
		settings.SetInt32("show_commentmargin", kSKShowCommentMargin);
	if (settings.FindInt32("mark_caretline", &intVal) != B_OK)
		settings.SetInt32("mark_caretline", kSKMarkCaretLine);
	if (settings.FindInt32("show_edgeline", &intVal) != B_OK)
		settings.SetInt32("show_edgeline", kSKShowEdgeLine);
	if (settings.FindString("edgeline_column", &stringVal) != B_OK)
		settings.SetString("edgeline_column", kSKEdgeLineColumn);
	if (settings.FindInt32("enable_folding", &intVal) != B_OK)
		settings.SetInt32("enable_folding", kSKEnableFolding);
	//  Notifications Page
	if (settings.FindInt32("enable_notifications", &intVal) != B_OK)
		settings.SetInt32("enable_notifications", kSKEnableNotifications);
	//  Build Page
	if (settings.FindInt32("wrap_console", &intVal) != B_OK)
		settings.SetInt32("wrap_console", kSKWrapConsole);
	if (settings.FindInt32("console_banner", &intVal) != B_OK)
		settings.SetInt32("console_banner", kSKConsoleBanner);
	
	if (settings.FindInt32("editor_zoom", &intVal) != B_OK)
		settings.SetInt32("editor_zoom", kSKEditorZoom);
		
	if (settings.FindBool("find_wrap", &boolVal) != B_OK)
		settings.SetBool("find_wrap", kSKFindWrap);

	if (settings.FindBool("find_whole_word", &boolVal) != B_OK)
		settings.SetBool("find_whole_word", kSKFindWholeWord);

	if (settings.FindBool("find_match_case", &boolVal) != B_OK)
		settings.SetBool("find_match_case", kSKFindMatchCase);
	
	if (settings.FindInt32("log_destination", &intVal) != B_OK)
		settings.SetInt32("log_destination", 0);


	// Managed to get here without errors, reset counter and app version
	settings.SetInt64("last_used", real_time_clock());
	// Reset counter
	if ((status = settings.SetInt32("use_count", 0)) != B_OK)
		return status;
	// Reset app version
	return settings.SetBString("app_version", GenioNames::GetVersionInfo());
}





} // namespace GenioNames


