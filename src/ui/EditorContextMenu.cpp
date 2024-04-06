/*
 * Copyright 2023, Andrea Anzani 
 * Copyright 2024, Nexus6 
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

	sMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_RENAME, sMenu);

	sMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_FIND_IN_BROWSER, sMenu);
}


void
EditorContextMenu::Show(Editor* editor, BPoint point)
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
		LSPDiagnostic dia;
		BPoint p = editor->ConvertFromScreen(point);
		Sci_Position sci_position = editor->SendMessage(SCI_POSITIONFROMPOINT, p.x, p.y);
		int32 index = lsp->DiagnosticFromPosition(sci_position, dia);
		if ( index > -1 && dia.diagnostic.codeActions.value().size() > 0) {
			sFixMenu->RemoveItems(0, sFixMenu->CountItems(), true);
			std::vector<CodeAction> actions = dia.diagnostic.codeActions.value();
			for (int i = 0; i < static_cast<int>(actions.size()); i++) {
				auto item = new BMenuItem(actions[i].title.c_str(),
					new GMessage({{"what", kApplyFix}, {"index", index}, {"action", i}, {"quickFix", true}}));
				sFixMenu->AddItem(item);
				menu = sFixMenu;
				menu->SetTargetForItems((BHandler*)editor);
				lsp->EndHover();
			}
		}
	}

	if (menu == sMenu) {
		ActionManager::SetEnabled(B_CUT,   editor->CanCut());
		ActionManager::SetEnabled(B_COPY,  editor->CanCopy());
		ActionManager::SetEnabled(B_PASTE, editor->CanPaste());
	}

	bool isFindInBrowserEnable = ActionManager::IsEnabled(MSG_FIND_IN_BROWSER);
	ActionManager::SetEnabled(MSG_FIND_IN_BROWSER, (editor->GetProjectFolder() != nullptr));

	menu->Go(BPoint(point.x, point.y), true);

	ActionManager::SetEnabled(MSG_FIND_IN_BROWSER, isFindInBrowserEnable);
}
