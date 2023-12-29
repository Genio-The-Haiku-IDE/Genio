/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EditorTabManager_H
#define EditorTabManager_H


#include <SupportDefs.h>
#include "TabManager.h"

class Editor;

class EditorTabManager : public TabManager {
public:
	EditorTabManager(const BMessenger& target);

	Editor*		EditorAt(int32 index);
	Editor*		SelectedEditor();
	Editor*		EditorBy(entry_ref* ref);

	BString		GetToolTipText(int32 index);

private:

};


#endif // _H
