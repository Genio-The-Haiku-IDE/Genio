/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
/**
   This is what a project preferences file may contain

// These are "static" data read once, when the project is opened


BString "project_extensioned_name"			// Project name with .idmpro
BString "project_name"						// Project name
BString "project_directory"					// Base directory
BString "project_type" (c++, cargo, haiku_source)
bool    "run_in_terminal"

 
// These are "runtime" data read from file when requested
// Normally set in NewProjectWindow class or in Project->Settings
// "project_source" and "project_file" set in ProjectParser class
// "parseless_item" set in context menu: Exclude File
// "release_mode" set in menu Build->Build mode

BString "project_target"					// Executable path
											// or base directory in cargo
BString "project_build_command"
BString "project_clean_command"
BString "project_scm"
BString "project_file"   []
BString "project_source" []
BString "parseless_item" []	 				// an excluded file or source
BString "project_run_args" []	 			// run arguments
bool    "release_mode"

Possible future settings
BString "parseless_dirs"  []
BString "parseless_dirs_startchars"  []
BString "parseless_extensions" []
 */
#ifndef PROJECT_PARSER_H
#define PROJECT_PARSER_H


#include <String.h>
#include <algorithm>
#include <string>
#include <vector>

#include "TPreferences.h"

class ProjectParser {
public:
								ProjectParser(TPreferences* prefs);
								~ProjectParser();

			status_t			ParseProjectFiles(BString directory);

private:
			int32				_GetParselessItems();
			void				_ScanDirectory(BString directory);
			int32				_SetParselessItems();

			TPreferences*		fPreferences;

	static const std::vector<std::string> source_extensions;
	static const std::vector<std::string> excluded_extensions;
	    std::vector<BString> 	parseless_items;
	    std::vector<BString> 	parseless_found;
	    BString					fProjectFullName;
	    BString					fProjectType;
	    int32					fParselessIn;
	    int32					fParselessOut;
	    int32					fSourcesCount;
	    int32					fFilesCount;
};


#endif // PROJECT_PARSER_H
