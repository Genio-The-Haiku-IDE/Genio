/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GTabEditor.h"
#include "EditorTabView.h"
#include <Message.h>


BSize
GTabEditor::MinSize()
{
	BSize size(GTabCloseButton::MinSize());
	size.width = 200.0f;
	return size;
}



BSize
GTabEditor::MaxSize()
{
	BSize size(GTabCloseButton::MaxSize());
	float extra = be_control_look->DefaultLabelSpacing();
	float labelWidth = 300.0f;
	size.width = labelWidth + extra;
	return size;
}


void
GTabEditor::CloseButtonClicked()
{
	EditorTabView* tabView = dynamic_cast<EditorTabView*>(Container()->GetGTabView());
	if (tabView) {
		BMessage msg(EditorTabView::kETVCloseTab);
		msg.AddInt32("index", Container()->IndexOfTab(this));
		BMessenger(Handler()).SendMessage(&msg);
	}
}


void
GTabEditor::MouseDown(BPoint where)
{
	BMessage* msg = Window()->CurrentMessage();
	if (msg == nullptr)
		return;

	const int32 buttons = msg->GetInt32("buttons", 0);
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		EditorTabView* tabView = dynamic_cast<EditorTabView*>(Container()->GetGTabView());
		if (tabView) {
			ConvertToScreen(&where);
			tabView->ShowTabMenu(this, where);
		}
		return;
	}
	GTabCloseButton::MouseDown(where);
}


void
GTabEditor::MouseMoved(BPoint where, uint32 transit,
										const BMessage* dragMessage)
{
	GTabCloseButton::MouseMoved(where, transit, dragMessage);
	if (transit == B_ENTERED_VIEW) {
		UpdateToolTip();
	}
}

void
GTabEditor::SetColor(const rgb_color& color)
{
	fColor = color;
	UpdateToolTip();
}


void
GTabEditor::SetLabel(const char* label)
{
	GTabCloseButton::SetLabel(label);
	UpdateToolTip();
}


void
GTabEditor::UpdateToolTip()
{
	EditorTabView* tabView = dynamic_cast<EditorTabView*>(Container()->GetGTabView());
	if (tabView) {
		SetToolTip(tabView->GetToolTipText(this).String());
	}
}