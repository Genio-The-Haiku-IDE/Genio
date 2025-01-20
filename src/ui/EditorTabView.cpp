/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EditorTabView.h"
#include <Box.h>
#include "Editor.h"
#include "Log.h"
#include "TabsContainer.h"
#include "GTab.h"

EditorTabView::EditorTabView(BMessenger target):GTabView("_editor_tabview_",
										'EDTV',
										B_HORIZONTAL,
										true,
										true), fTarget(target)
{

}


void
EditorTabView::AddEditor(const char* label, Editor* editor, BMessage* info, int32 index)
{
	AddTab(label, new BBox("fakeEditor"), index);

	// Assuming nothing went wrong ...
	BMessage message;
	if (info)
		message = *info;
	message.what = kETBNewTab;
	message.AddRef("ref", editor->FileRef());

	fTarget.SendMessage(&message);
}



Editor*
EditorTabView::EditorBy(const entry_ref* ref) const
{
	BEntry entry(ref, true);
	int32 filesCount = Container()->CountTabs();
	for (int32 index = 0; index < filesCount; index++) {
		Editor* editor = dynamic_cast<Editor*>(Container()->TabAt(index));
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


void
EditorTabView::SetTabColor(const entry_ref* ref, const rgb_color& color)
{
	printf("TODO\n");
}


void
EditorTabView::SelectTab(const entry_ref* ref, BMessage* selInfo)
{
	printf("TODO\n");
	//GTabView::SelectTab();
}
