/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "EditorContextMenu.h"

#include "Editor.h"
#include "GenioWindowMessages.h"

#include <Autolock.h>
#include <Catalog.h>
#include <PopUpMenu.h>
#include <MenuItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioWindow"


BPopUpMenu* EditorContextMenu::sMenu = NULL;

EditorContextMenu::EditorContextMenu()
{
}

/* static */
void 
EditorContextMenu::AddToPopUp(const char *label, uint32 what, bool enabled)
{

	if(label[0] == 0)
		sMenu->AddSeparatorItem();
	else {
		BMessage *message = new BMessage(what);
		BMenuItem *item = new BMenuItem(B_TRANSLATE(label), message);
		item->SetEnabled(enabled);
		sMenu->AddItem(item);
	}
}

void
EditorContextMenu::Show(Editor* editor, BPoint point)
{
	BAutolock l(editor->Looper());
	if (sMenu)
		delete sMenu;

	sMenu = new BPopUpMenu("EditorContextMenu", false, false);
	
	bool writable = !editor->IsReadOnly();
	AddToPopUp("Undo",   B_UNDO, writable && editor->CanUndo());
	AddToPopUp("Redo",   B_REDO, writable && editor->CanRedo());
	AddToPopUp("");
	AddToPopUp("Cut",	 B_CUT,  writable && editor->CanCut());
	AddToPopUp("Copy",	 B_COPY, editor->CanCopy());
	AddToPopUp("Paste",	 B_PASTE,editor->CanPaste());
	AddToPopUp("Delete", MSG_TEXT_DELETE, editor->CanClear());
	AddToPopUp("");
	AddToPopUp("Select All", B_SELECT_ALL);
	sMenu->SetTargetForItems((BHandler*)editor->Window());
	sMenu->Go(BPoint(point.x, point.y), true);
}
