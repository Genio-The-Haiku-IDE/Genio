/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EditorMessages_H
#define EditorMessages_H


enum {
	kApplyFix			= 'Fixy'
};

enum {
	EDITOR_FIND_COUNT				= 'Efco',
	EDITOR_FIND_NEXT_MISS			= 'Efnm',
	EDITOR_FIND_PREV_MISS			= 'Efpm',
	EDITOR_FIND_SET_MARK			= 'Efsm',
	EDITOR_POSITION_CHANGED			= 'Epch',
	EDITOR_REPLACE_ONE				= 'Eron',
	EDITOR_REPLACE_ALL_COUNT		= 'Erac',
	EDITOR_UPDATE_SAVEPOINT			= 'EUSP',
	EDITOR_UPDATE_DIAGNOSTICS		= 'diag'
};

#endif // EditorMessages_H
