/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EditorContextMenu_H
#define EditorContextMenu_H


#include <Point.h>
#include <SupportDefs.h>

class CodeEditor;
class BPopUpMenu;
class BMenuItem;

class EditorContextMenu {
public:
	static void Show(CodeEditor*, BPoint point);

private:
		   EditorContextMenu();

	static BPopUpMenu*	sMenu;
	static BPopUpMenu*	sFixMenu;
	static void _CreateMenu();

};


#endif // EditorContextMenu_H
