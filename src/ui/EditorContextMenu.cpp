/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "EditorContextMenu.h"

#include "CodeEditor.h"
#include "GenioWindowMessages.h"

#include <Autolock.h>
#include <Catalog.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include "ActionManager.h"
#include "LSPEditorWrapper.h"
#include "GMessage.h"
#include "EditorMessages.h"

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GenioWindow"


BPopUpMenu* EditorContextMenu::sMenu = nullptr;
BPopUpMenu* EditorContextMenu::sFixMenu = nullptr;

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
EditorContextMenu::Show(CodeEditor* editor, BPoint point)
{
	BAutolock l(editor->Looper());

	if (!sMenu)
		_CreateMenu();

	if (!sFixMenu) {
		sFixMenu = new BPopUpMenu("FixEditorContextMenu", false, false);
		sFixMenu->AddItem(new BMenuItem("Fix!", nullptr));
	}

	BPopUpMenu* menu = sMenu;
	//NOTE: the target should always be editor (for all messages) but we need fist to move them
	//from the window.
	menu->SetTargetForItems((BHandler*)editor->Window());

	LSPEditorWrapper* lsp = editor->GetLSPEditorWrapper();
	if (lsp) {
		LSPEditorWrapper::LSPDiagnostic dia;
		BPoint p = editor->ConvertFromScreen(point);
		Sci_Position sci_position = editor->SendMessage(SCI_POSITIONFROMPOINT, p.x, p.y);
		int32 index = lsp->DiagnosticFromPosition(sci_position, dia);
		if ( index > -1 && dia.diagnostic.codeActions.value().size() > 0) {
			sFixMenu->ItemAt(0)->SetLabel(dia.fixTitle.c_str());
			sFixMenu->ItemAt(0)->SetMessage(new GMessage({{"what",kApplyFix},{"index", index},
														  {"quickFix", true}}));
			menu = sFixMenu;
			menu->SetTargetForItems((BHandler*)editor);
			lsp->EndHover();
		}
	}

	if (menu == sMenu) {
		ActionManager::SetEnabled(B_CUT,   editor->CanCut());
		ActionManager::SetEnabled(B_COPY,  editor->CanCopy());
		ActionManager::SetEnabled(B_PASTE, editor->CanPaste());
	}

	menu->Go(BPoint(point.x, point.y), true);
}
