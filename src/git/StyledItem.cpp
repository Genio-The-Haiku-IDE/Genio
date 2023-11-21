/*
 * Copyright 2023 Nexus6 (nexus6.haiku@icloud.com)
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StyledItem.h"

#include <Bitmap.h>
#include <ControlLook.h>
#include <Font.h>
#include <NodeInfo.h>
#include <StringItem.h>
#include <View.h>
#include <Window.h>

#include "Utils.h"

class TemporaryTextControl: public BTextControl {
	typedef	BTextControl _inherited;


public:
	StyledItem *fStyledItem;

	TemporaryTextControl(BRect frame, const char* name, const char* label, const char* text,
							BMessage* message, StyledItem *item,
							uint32 resizingMode = B_FOLLOW_LEFT|B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW|B_NAVIGABLE)
		:
		BTextControl(frame, name, label, text, message, resizingMode, flags),
		fStyledItem(item)
	{
		SetEventMask(B_POINTER_EVENTS|B_KEYBOARD_EVENTS);
	}

	virtual void AllAttached()
	{
		TextView()->SelectAll();
	}

	virtual void MouseDown(BPoint point)
	{
		if (Bounds().Contains(point))
			_inherited::MouseDown(point);
		else {
			fStyledItem->AbortRename();
		}
		Invoke();
	}

	virtual void KeyDown(const char* bytes, int32 numBytes)
	{
		if (numBytes == 1 && *bytes == B_ESCAPE) {
			fStyledItem->AbortRename();
		}
		if (numBytes == 1 && *bytes == B_RETURN) {
			fStyledItem->CommitRename();
		}
	}
};


StyledItem::StyledItem(BOutlineListView *container,
						const char* text,
						const uint32 item_type,
						uint32 outlineLevel,
						bool expanded,
						const char *iconName)
	:
	BStringItem(text, outlineLevel, expanded),
	fContainerListView(container),
	fFirstTimeRendered(true),
	fInitRename(false),
	fMessage(nullptr),
	fTextControl(nullptr),
	fToolTipText(nullptr),
	fPrimaryTextStyle(B_REGULAR_FACE),
	fSecondaryTextStyle(B_REGULAR_FACE),
	fIconName(iconName),
	fItemType(item_type)
{
}


StyledItem::~StyledItem()
{
	delete fTextControl;
}


void
StyledItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
	if (Text() == NULL)
		return;

	if (IsSelected() || complete) {
		rgb_color oldLowColor = owner->LowColor();
		rgb_color color;
		if (IsSelected())
			color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			color = owner->ViewColor();
		owner->SetLowColor(color);
		owner->FillRect(bounds, B_SOLID_LOW);
		owner->SetLowColor(oldLowColor);
	}

	owner->SetFont(be_plain_font);


	if (IsSelected())
		owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
	else
		owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	BBitmap *icon = nullptr;
	float iconSize = 0;
	if (fIconName != nullptr) {
		iconSize = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
		icon = new BBitmap(BRect(iconSize - 1.0f), 0, B_RGBA32);
		GetVectorIcon(fIconName.String(), icon);
	}
	BPoint iconStartingPoint(bounds.left + 4.0f, bounds.top  + (bounds.Height() - iconSize) / 2.0f);
	if (icon != nullptr) {
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->DrawBitmapAsync(icon, iconStartingPoint);
	}

	// Check if there is an InitRename request and show a TextControl
	if (fInitRename) {
		BRect textRect;
		textRect.top = bounds.top - 0.5f;
		textRect.left = iconStartingPoint.x + iconSize + be_control_look->DefaultLabelSpacing();
		textRect.bottom = bounds.bottom - 1;
		textRect.right = bounds.right;
		_DrawTextWidget(owner, textRect);
	} else {
		BPoint textPoint(iconStartingPoint.x + iconSize + be_control_look->DefaultLabelSpacing(),
						bounds.top + BaselineOffset());
		_DrawText(owner, textPoint);

		owner->Sync();

		if (fFirstTimeRendered) {
			owner->Invalidate();
			fFirstTimeRendered = false;
		}
	}
}


void
StyledItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, font);
}


void
StyledItem::InitRename(BMessage* message)
{
	if (fTextControl != nullptr)
		debugger("StyledItem::InitRename() called twice!");
	fInitRename = true;
	fMessage = message;
}


void
StyledItem::AbortRename()
{
	if (fTextControl != nullptr)
		_DestroyTextWidget();
	fInitRename = false;
}


void
StyledItem::CommitRename()
{
	if (fTextControl != nullptr) {
		fMessage->AddString("_value", fTextControl->Text());
		BMessenger(fTextControl->Parent()).SendMessage(fMessage);
		SetText(fTextControl->Text());
		_DestroyTextWidget();
	}
	fInitRename = false;
}


void
StyledItem::_DrawText(BView* owner, const BPoint& point)
{
	BFont font;
	owner->GetFont(&font);
	font.SetFace(fPrimaryTextStyle);
	owner->SetFont(&font);

	owner->SetDrawingMode(B_OP_COPY);
	owner->MovePenTo(point);
	owner->DrawString(Text());

	font.SetFace(fSecondaryTextStyle);
	owner->SetFont(&font);

	if (!fSecondaryText.IsEmpty()) {
		BString text;
		text << fSecondaryText.String();
		// Apply any style change here (i.e. bold, italic)
		owner->DrawString(text.String());
	}

	owner->Sync();
}


void
StyledItem::_DrawTextWidget(BView* owner, const BRect& textRect)
{
	if (fTextControl == nullptr) {
		fTextControl = new TemporaryTextControl(textRect, "RenameTextWidget",
											"", Text(), fMessage, this,
											B_FOLLOW_NONE);
		owner->AddChild(fTextControl);
		fTextControl->TextView()->SetAlignment(B_ALIGN_LEFT);
		fTextControl->SetDivider(0);
		fTextControl->TextView()->SelectAll();
		fTextControl->TextView()->ResizeBy(0, -3);
	}
	fTextControl->MakeFocus();
}


void
StyledItem::_DestroyTextWidget()
{
	if (fTextControl != nullptr) {
		fTextControl->RemoveSelf();
		delete fTextControl;
		fTextControl = nullptr;
	}
}