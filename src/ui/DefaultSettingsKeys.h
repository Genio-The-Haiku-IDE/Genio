/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DEFAULT_SETTINGS_KEYS_H
#define DEFAULT_SETTINGS_KEYS_H

#include <Control.h>

// General settings (SK: SettingsKey)

const BString kSKProjectsDirectory {"/boot/home/workspace"};// "projects_directory"
constexpr int32 kSKFullPathTitle {B_CONTROL_ON};	// "fullpath_title"

const int32 kSKReopenProjects = B_CONTROL_ON;		// "reopen_projects"
const int32 kSKReopenFiles = B_CONTROL_ON;			// "reopen_files"
const int32 kSKShowProjects = B_CONTROL_ON;			// "show_projects"
const int32 kSKShowOutput = B_CONTROL_ON;			// "show_output"
const int32 kSKShowToolBar = B_CONTROL_ON;			// "show_toolbar"

const int32 kSKEditorFontSize = 13;					// "edit_fontsize"
const int32 kSKSyntaxHighlight = B_CONTROL_ON;		// "syntax_highlight"
const int32 kSKTabWidth = 4;						// "tab_width"
const int32 kSKBraceMatch = B_CONTROL_ON;			// "brace_match"
const int32 kSKSaveCaret = B_CONTROL_ON;			// "save_caret"

const int32 kSKShowLineNumber = B_CONTROL_ON;		// "show_linenumber"
const int32 kSKShowCommentMargin = B_CONTROL_ON;	// "show_commentmargin"
const int32 kSKMarkCaretLine = B_CONTROL_ON;		// "mark_caretline"
const int32 kSKShowEdgeLine = B_CONTROL_ON;			// "show_edgeline"
const BString kSKEdgeLineColumn = "80";				//"edgeline_column"
const int32 kSKEnableFolding = B_CONTROL_ON;		// "enable_folding"

const int32 kSKEnableNotifications = B_CONTROL_ON;	// "enable_notifications"

const int32 kSKWrapConsole = B_CONTROL_OFF;			// "wrap_console"
const int32 kSKConsoleBanner = B_CONTROL_ON;		// "console_banner"
const int32 kSKEditorZoom = 0;						// "editor_zoom"
const int32 kSKFindWrap = false;					// "find_wrap"
const int32 kSKFindWholeWord = false;				// "find_whole_word"
const int32 kSKFindMatchCase = false;				// "find_match_case"


#endif // DEFAULT_SETTINGS_KEYS_H
