/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EditorContextMenu_H
#define EditorContextMenu_H


#include <Point.h>
#include <SupportDefs.h>

class Editor;
class BPopUpMenu;

class EditorContextMenu {
public:
	static void Show(Editor*, BPoint point);
private:
		   EditorContextMenu();
	static void AddToPopUp(const char *label, uint32 what = 0, bool enabled = true);
	static BPopUpMenu*	sMenu;
};


#endif // EditorContextMenu_H
