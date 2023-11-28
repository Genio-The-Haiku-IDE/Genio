/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EditorTabManager.h"
#include "Editor.h"
#include "Log.h"
#include "ProjectFolder.h"

#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "EditorTabManager"


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
	if (sel >= 0 && sel < CountTabs())
		return EditorAt(sel);

	return nullptr;
}

Editor*
EditorTabManager::EditorBy(entry_ref* ref)
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

BString
EditorTabManager::GetToolTipText(int32 index)
{
	BString label("");
	Editor* editor = EditorAt(index);
	if (editor) {
		label << editor->Name();
		ProjectFolder* project = editor->GetProjectFolder();
		if (project) {
			label << "\n" << B_TRANSLATE("Project") << ": " << project->Name();
		}
	}
	return label;
}