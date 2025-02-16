/*
 * Copyright 2025, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "GTabEditor.h"

#include <Message.h>

#include "Editor.h"
#include "EditorTabView.h"


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
		msg.AddUInt64(kEditorId, fEditor->Id());
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
GTabEditor::DrawLabel(BView* owner, BRect frame, const BRect& updateRect, bool isFront)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	DrawCircle(owner, frame);
	be_control_look->DrawLabel(owner, fLabel.String(), frame, updateRect,
		base, 0, BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
}


void
GTabEditor::DrawCircle(BView* owner, BRect& frame)
{
	BRect circleFrame(frame);
	circleFrame.OffsetBy(0, 1);
	circleFrame.right = circleFrame.left + circleFrame.Height();
	circleFrame.InsetBy(4, 4);
	owner->SetHighColor(fColor);
	owner->FillEllipse(circleFrame);
	owner->SetHighColor(tint_color(fColor, B_DARKEN_1_TINT));
	owner->StrokeEllipse(circleFrame);
	frame.left = circleFrame.right + be_control_look->DefaultLabelSpacing();
}


void
GTabEditor::SetColor(const rgb_color& color)
{
	fColor = color;
	Invalidate();
}


void
GTabEditor::SetLabel(const char* label)
{
	GTabCloseButton::SetLabel(label);
	Invalidate();
}


void
GTabEditor::UpdateToolTip()
{
	EditorTabView* tabView = dynamic_cast<EditorTabView*>(Container()->GetGTabView());
	if (tabView) {
		SetToolTip(tabView->GetToolTipText(this).String());
	}
}
