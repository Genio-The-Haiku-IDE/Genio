/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GenioWindowMessages_H
#define GenioWindowMessages_H


// Self enum names begin with MSG_ and values are all lowercase
// External enum names begin with MODULENAME_ and values are Capitalized

enum {
	// Project menu
	MSG_PROJECT_CLOSE			= 'prcl',
	MSG_PROJECT_OPEN			= 'prop',
	MSG_PROJECT_OPEN_REMOTE		= 'pror',
	MSG_PROJECT_SET_ACTIVE		= 'psac',	// TODO
	MSG_PROJECT_SETTINGS		= 'prse',
	MSG_PROJECT_FOLDER_OPEN		= 'pfop',

	// File menu
	MSG_FILE_NEW				= 'fine',
	MSG_FILE_OPEN				= 'fiop',
	MSG_FILE_SAVE				= 'fisa',
	MSG_FILE_SAVE_AS			= 'fsas',
	MSG_FILE_SAVE_ALL			= 'fsal',
	MSG_FILE_CLOSE				= 'ficl',
	MSG_FILE_CLOSE_ALL			= 'fcal',
	MSG_FILE_FOLD_TOGGLE		= 'fifo',

	// Edit menu
	MSG_TEXT_DELETE				= 'tede',
	MSG_TEXT_OVERWRITE			= 'teov',
	MSG_WHITE_SPACES_TOGGLE		= 'whsp',
	MSG_LINE_ENDINGS_TOGGLE		= 'lien',
	MSG_DUPLICATE_LINE			= 'duli',
	MSG_DELETE_LINES			= 'deli',
	MSG_COMMENT_SELECTED_LINES	= 'cosl',
	MSG_EOL_CONVERT_TO_UNIX		= 'ectu',
	MSG_EOL_CONVERT_TO_DOS		= 'ectd',
	MSG_EOL_CONVERT_TO_MAC		= 'ectm',
	MSG_FILE_TRIM_TRAILING_SPACE = 'trim',

  MSG_AUTOCOMPLETION			= 'auto',
	MSG_FORMAT					= 'form',
	MSG_GOTODEFINITION			= 'gode',
	MSG_GOTODECLARATION			= 'gocl',
	MSG_GOTOIMPLEMENTATION		= 'goim',
	MSG_SWITCHSOURCE			= 'swit',

	// view
	MSG_VIEW_ZOOMIN				= 'zoin',
	MSG_VIEW_ZOOMOUT			= 'zoou',
	MSG_VIEW_ZOOMRESET			= 'zore',

	// Search menu & group
	MSG_FIND_GROUP_SHOW			= 'figs',
	MSG_FIND_MENU_SELECTED		= 'fmse',
	MSG_FIND_PREVIOUS			= 'fipr',
	MSG_FIND_MARK_ALL			= 'fmal',
	MSG_FIND_NEXT				= 'fite',
	MSG_FIND_INVOKED			= 'finv',
	MSG_REPLACE_GROUP_SHOW		= 'regs',
	MSG_REPLACE_MENU_SELECTED 	= 'rmse',
	MSG_REPLACE_ONE				= 'reon',
	MSG_REPLACE_NEXT			= 'rene',
	MSG_REPLACE_PREVIOUS		= 'repr',
	MSG_REPLACE_ALL				= 'real',
	MSG_GOTO_LINE				= 'goli',
	MSG_BOOKMARK_CLEAR_ALL		= 'bcal',
	MSG_BOOKMARK_GOTO_NEXT		= 'bgne',
	MSG_BOOKMARK_GOTO_PREVIOUS	= 'bgpr',
	MSG_BOOKMARK_TOGGLE			= 'book',

	// Build menu
	MSG_BUILD_PROJECT			= 'bupr',
	MSG_BUILD_PROJECT_STOP		= 'bpst',
	MSG_CLEAN_PROJECT			= 'clpr',
	MSG_RUN_TARGET				= 'ruta',
	MSG_BUILD_MODE_RELEASE		= 'bmre',
	MSG_BUILD_MODE_DEBUG		= 'bmde',
	MSG_DEBUG_PROJECT			= 'depr',
	MSG_MAKE_CATKEYS			= 'maca',
	MSG_MAKE_BINDCATALOGS		= 'mabi',

	// Scm menu
	MSG_GIT_COMMAND				= 'gitc',
	MSG_GIT_SWITCH_BRANCH		= 'gtsb',
	MSG_HG_COMMAND				= 'hgco',

	// Window menu
	MSG_WINDOW_SETTINGS			= 'wise',
	MSG_TOGGLE_TOOLBAR			= 'toto',


	// Toolbar
	MSG_BUFFER_LOCK					= 'bulo',
	MSG_FILE_MENU_SHOW				= 'fmsh',
	MSG_FILE_NEXT_SELECTED			= 'fnse',
	MSG_FILE_PREVIOUS_SELECTED		= 'fpse',
	MSG_FIND_GROUP_TOGGLED			= 'figt',
	MSG_FIND_IN_FILES				= 'fifi',
	MSG_RUN_CONSOLE_PROGRAM_SHOW	= 'rcps',
	MSG_RUN_CONSOLE_PROGRAM			= 'rcpr',

	MSG_REPLACE_GROUP_TOGGLED		= 'regt',
	MSG_SHOW_HIDE_PROJECTS			= 'shpr',
	MSG_SHOW_HIDE_OUTPUT			= 'shou',
	MSG_SELECT_TAB					= 'seta',

	MSG_ESCAPE_KEY					= 'escp',

	MSG_SHOW_TEMPLATE_USER_FOLDER	= 'stuf',
	MSG_CREATE_NEW_PROJECT			= 'mcnp',

	MSG_FIND_WRAP					= 'fiwr',
	MSG_FIND_WHOLE_WORD				= 'fiww',
	MSG_FIND_MATCH_CASE				= 'fimc'
};


// "notification" messages
enum {
	MSG_NOTIFY_EDITOR_FILE_OPENED 		= 'efop',	// file_name (string)
	MSG_NOTIFY_EDITOR_FILE_CLOSED 		= 'efcx',	// file_name (string)
	MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED = 'stch',	// file_name (string)
													// needs_save (bool)

	MSG_NOTIFY_BUILDING_PHASE			= 'blph',	// building (bool)
	MSG_NOTIFY_PROJECT_SET_ACTIVE		= 'npsa'	// active_project (pointer)
};

#endif // GenioWindowMessages_H
