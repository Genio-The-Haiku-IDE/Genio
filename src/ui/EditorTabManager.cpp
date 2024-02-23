/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EditorTabManager.h"
#include "Editor.h"
#include "Log.h"
#include "ProjectFolder.h"
#include "ActionManager.h"
#include "GenioWindowMessages.h"
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "EditorTabManager"

EditorTabManager::EditorTabManager(const BMessenger& target) : TabManager(target), fPopUpMenu(nullptr)
{
	fPopUpMenu = new BPopUpMenu("tabmenu", false, false, B_ITEMS_IN_COLUMN);
	ActionManager::AddItem(MSG_FILE_CLOSE, 	fPopUpMenu);
	ActionManager::AddItem(MSG_FILE_CLOSE_ALL, fPopUpMenu);
	ActionManager::AddItem(MSG_FILE_CLOSE_OTHER, fPopUpMenu);

	fPopUpMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_FIND_IN_BROWSER, fPopUpMenu);

	fPopUpMenu->SetTargetForItems(target);
}

EditorTabManager::~EditorTabManager()
{
	delete fPopUpMenu;
}

Editor*
EditorTabManager::EditorAt(int32 index) const
{
	return dynamic_cast<Editor*>(ViewForTab(index));
}


Editor*
EditorTabManager::SelectedEditor() const
{
	int32 sel = SelectedTabIndex();
	if (sel >= 0 && sel < CountTabs())
		return EditorAt(sel);

	return nullptr;
}


Editor*
EditorTabManager::EditorBy(const entry_ref* ref) const
{
	BEntry entry(ref, true);
	int32 filesCount = CountTabs();
	for (int32 index = 0; index < filesCount; index++) {
		Editor* editor = EditorAt(index);
		if (editor == nullptr) {
			BString notification;
			notification
				<< "Index " << index << ": NULL editor pointer";
			LogInfo(notification.String());
			continue;
		}

		BEntry matchEntry(editor->FileRef(), true);
		if (matchEntry == entry)
			return editor;
	}
	return nullptr;
}


Editor*
EditorTabManager::EditorBy(const node_ref* nodeRef) const
{
	int32 filesCount = CountTabs();
	for (int32 index = 0; index < filesCount; index++) {
		Editor* editor = EditorAt(index);
		if (editor == nullptr) {
			BString notification;
			notification
				<< "Index " << index << ": NULL editor pointer";
			LogInfo(notification.String());
			continue;
		}
		if (editor->NodeRef() != nullptr && *editor->NodeRef() == *nodeRef)
			return editor;
	}
	return nullptr;
}


BString
EditorTabManager::GetToolTipText(int32 index)
{
	BString label("");
	Editor* editor = EditorAt(index);
	if (editor) {
		label << editor->FilePath();
		ProjectFolder* project = editor->GetProjectFolder();
		if (project) {
			if (label.StartsWith(project->Path()))
				label.Remove(0, project->Path().Length() + 1);
					// Length + 1 to also remove the path separator
			label << "\n" << B_TRANSLATE("Project") << ": " << project->Name();
			if (project->Active())
				label << " (" << B_TRANSLATE("Active") << ")";
		}
	}
	return label;
}

void
EditorTabManager::ShowTabMenu(BMessenger target, BPoint where, int32 index)
{
	for (int32 i=0;i<fPopUpMenu->CountItems();i++) {
		if (fPopUpMenu->ItemAt(i)->Message() != nullptr)
			fPopUpMenu->ItemAt(i)->Message()->SetInt32("tab_index", index);
	}


	bool isFindInBrowserEnable = ActionManager::IsEnabled(MSG_FIND_IN_BROWSER);
	ActionManager::SetEnabled(MSG_FIND_IN_BROWSER, (EditorAt(index) != nullptr && EditorAt(index)->GetProjectFolder() != nullptr));

	fPopUpMenu->Go(where, true);

	ActionManager::SetEnabled(MSG_FIND_IN_BROWSER, isFindInBrowserEnable);
}
