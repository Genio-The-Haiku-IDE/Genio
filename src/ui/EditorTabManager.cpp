/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EditorTabManager.h"
#include "Editor.h"

EditorTabManager::EditorTabManager(const BMessenger& target) : TabManager(target)
{
}

Editor*		
EditorTabManager::EditorAt(int32 index)
{
	return dynamic_cast<Editor*>(ViewForTab(index));
}

Editor*		
EditorTabManager::SelectedEditor()
{
	int32 sel = SelectedTabIndex();
	if (sel >0 && sel < CountTabs())
		return EditorAt(sel);
	
	return NULL;
}