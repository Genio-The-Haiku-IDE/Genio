/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "EditorContextMenu.h"

#include "Editor.h"
#include "GenioWindowMessages.h"

#include <Autolock.h>
#include <Catalog.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include "ActionManager.h"


#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioWindow"


BPopUpMenu* EditorContextMenu::sMenu = NULL;

EditorContextMenu::EditorContextMenu()
{

}

void
EditorContextMenu::_CreateMenu()
{
	sMenu = new BPopUpMenu("EditorContextMenu", false, false);
	
	ActionManager::AddItem(B_UNDO, sMenu);
	ActionManager::AddItem(B_REDO, sMenu);

	sMenu->AddSeparatorItem();

	ActionManager::AddItem(B_CUT, sMenu);
	ActionManager::AddItem(B_COPY, sMenu);
	ActionManager::AddItem(B_PASTE, sMenu);
	ActionManager::AddItem(MSG_TEXT_DELETE, sMenu);

	sMenu->AddSeparatorItem();

	ActionManager::AddItem(B_SELECT_ALL, sMenu);

	sMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_GOTODEFINITION, sMenu);
	ActionManager::AddItem(MSG_GOTODECLARATION, sMenu);
	ActionManager::AddItem(MSG_GOTOIMPLEMENTATION, sMenu);
}


void
EditorContextMenu::Show(Editor* editor, BPoint point)
{
	BAutolock l(editor->Looper());
	if (!sMenu)
		_CreateMenu();

	sMenu->SetTargetForItems((BHandler*)editor->Window());
	sMenu->Go(BPoint(point.x, point.y), true);
}
