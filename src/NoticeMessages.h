/*
 * Copyright 2025, Stefano Ceccherini
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

// "notification" messages
enum {
	// editor
	MSG_NOTIFY_EDITOR_FILE_OPENED 		= 'efop',	// file_name (string)

	MSG_NOTIFY_EDITOR_FILE_CLOSED 		= 'efcx',	// file_name (string)

	MSG_NOTIFY_EDITOR_POSITION_CHANGED	= 'epch',	// file_name (string)
													// line (int32)

	MSG_NOTIFY_FILE_SAVE_STATUS_CHANGED = 'stch',	// file_name (string)
													// needs_save (bool)

	// tab selected
	MSG_NOTIFY_EDITOR_FILE_SELECTED		= 'efsl',	// ref (ref)

	MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED	= 'esup',	// ref (ref)
													// symbols (BMessage)
													// status (int32)

	MSG_NOTIFY_BUILDING_PHASE			= 'blph',	// building (bool)
													// cmd_type (string)
													// project_name (string)
													// project_path (string)
													// status (int32)
	MSG_NOTIFY_LSP_INDEXING				= 'lsid',
	MSG_NOTIFY_PROJECT_LIST_CHANGED		= 'nplc',
	MSG_NOTIFY_PROJECT_SET_ACTIVE		= 'npsa',	// active_project_name (string)
													// active_project_path (string)
	MSG_NOTIFY_FIND_STATUS				= 'fist',

	// workspace
	MSG_NOTIFY_WORKSPACE_PREPARATION_STARTED = 'wkps',
	MSG_NOTIFY_WORKSPACE_PREPARATION_COMPLETED = 'wkpc',

	// git / source control
	MSG_NOTIFY_GIT_BRANCH_CHANGED = 'gbch'			// current_branch (string)
													// project_path (string)
};

