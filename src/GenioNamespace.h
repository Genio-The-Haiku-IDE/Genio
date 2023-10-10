/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef Genio_NAMESPACE_H
#define Genio_NAMESPACE_H

#include <Catalog.h>
#include <String.h>

namespace GenioNames
{
	const BString kApplicationName(B_TRANSLATE_SYSTEM_NAME("Genio"));
	const BString kApplicationSignature("application/x-vnd.Genio");
	const BString kSettingsFileName("Genio.settings");
	const BString kSettingsFilesToReopen("files_to_reopen.settings");
	const BString kSettingsProjectsToReopen("workspace.settings");
	const BString kUISettingsFileName("ui.settings");
	const BString kProjectExtension(".idmpro");
	const BString kProjectSettingsFile(".genio");

	int32 CompareVersion(const BString appVersion, const BString fileVersion);
	BString GetSignature();
	BString GetVersionInfo();
	status_t LoadSettingsVars();
	status_t SaveSettingsVars();
	status_t UpdateSettingsFile();

	typedef struct {
		BString projects_directory;
		int32 fullpath_title;
		int32 reopen_projects;
		int32 reopen_files;
		int32 show_projects;
		int32 show_output;
		int32 show_toolbar;
		int32 edit_fontsize;
		int32 syntax_highlight;
		int32 tab_width;
		int32 brace_match;
		int32 save_caret;
		int32 show_linenumber;
		int32 show_commentmargin;
		int32 mark_caretline;
		int32 show_edgeline;
		BString edgeline_column;
		int32 enable_folding;
		int32 enable_notifications;
		int32 wrap_console;
		int32 console_banner;
		int32 editor_zoom;
		bool  find_wrap;
		bool  find_whole_word;
		bool  find_match_case;
		int32 log_destination;
		int32 log_level;
		int32 trim_trailing_whitespace;
		bool	show_white_space = false;
		bool	show_line_endings = false;

	} SettingsVars;

	extern SettingsVars Settings;

}

#endif // Genio_NAMESPACE_H
