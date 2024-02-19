/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EditorTabManager.h"
#include "Editor.h"
#include "Log.h"
#include "ProjectFolder.h"
#include <MenuItem.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "EditorTabManager"

enum {
	MSG_CLOSE_TAB			= 'cltb',
	MSG_CLOSE_TABS_ALL		= 'clta',
	MSG_CLOSE_TABS_OTHER	= 'clto'
};


EditorTabManager::EditorTabManager(const BMessenger& target) : TabManager(target), fPopUpMenu(nullptr)
{
	fPopUpMenu = new BPopUpMenu("tabmenu", false, false, B_ITEMS_IN_COLUMN);

	BMessage* closeMessage = new BMessage(MSG_CLOSE_TAB);
	closeMessage->AddPointer("tab_source", this);
	BMenuItem* close = new BMenuItem("Close", closeMessage);

	BMessage* closeAllMessage = new BMessage(MSG_CLOSE_TABS_ALL);
	closeAllMessage->AddPointer("tab_source", this);
	BMenuItem* closeAll = new BMenuItem("Close all", closeAllMessage);

	BMessage* closeOtherMessage = new BMessage(MSG_CLOSE_TABS_OTHER);
	closeOtherMessage->AddPointer("tab_source", this);
	BMenuItem* closeOther = new BMenuItem("Close other", closeOtherMessage);

	fPopUpMenu->AddItem(close);
	fPopUpMenu->AddItem(closeAll);
	fPopUpMenu->AddItem(closeOther);
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
EditorTabManager::ShowTabMenu(BMessenger target, BPoint where)
{
		fPopUpMenu->SetTargetForItems(target);
		fPopUpMenu->Go(where, true);
}

void
EditorTabManager::HandleTabMenuAction(BMessage* message)
{
		switch (message->what) {
			case MSG_CLOSE_TAB:
			{
				int32 index = -1;
				if (message->FindInt32("tab_index", &index) == B_OK)
					CloseTabs(&index, 1);
				break;
			}
			case MSG_CLOSE_TABS_ALL: {
				int32 count = CountTabs();
				int32 tabsToClose[count];
				int32 added = 0;
				for (auto i = count - 1; i >= 0; i--) {
					tabsToClose[added++] = i;
				}
				CloseTabs(tabsToClose, added);
				break;
			}
			case MSG_CLOSE_TABS_OTHER: {
				int32 index = -1;
				int32 count = CountTabs();
				int32 tabsToClose[count];
				int32 added = 0;
				if (message->FindInt32("tab_index", &index) == B_OK) {
					for (auto i = count - 1; i >= 0; i--) {
						if (i != index)
							tabsToClose[added++] = i;
					}
					CloseTabs(tabsToClose, added);
				}
				break;
			}
			default:
				break;
		}
}