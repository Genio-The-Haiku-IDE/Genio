/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ProjectItem.h"

#include <Bitmap.h>
#include <ControlLook.h>
#include <Font.h>
#include <NodeInfo.h>
#include <StringItem.h>
#include <View.h>
#include <Window.h>

#include "IconCache.h"
#include "ProjectFolder.h"
#include "Utils.h"


class ProjectItem;

class TemporaryTextControl: public BTextControl {	
	typedef	BTextControl _inherited;
	
	
public:
	ProjectItem *fProjectItem;

	TemporaryTextControl(BRect frame, const char* name, const char* label, const char* text,
							BMessage* message, ProjectItem *item, uint32 resizingMode = B_FOLLOW_LEFT|B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW|B_NAVIGABLE)
		:
		BTextControl(frame, name, label, text, message, resizingMode, flags),
		fProjectItem(item)
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
			fProjectItem->AbortRename();
		}
		Invoke();
	}
	
	virtual void KeyDown(const char* bytes, int32 numBytes)
	{
		if (numBytes == 1 && *bytes == B_ESCAPE) {
			fProjectItem->AbortRename();
		}
		if (numBytes == 1 && *bytes == B_RETURN) {
			fProjectItem->CommitRename();
		}
	}
};


ProjectItem::ProjectItem(SourceItem *sourceItem)
	:
	BStringItem(sourceItem->Name()),
	fSourceItem(sourceItem),
	fFirstTimeRendered(true),
	fInitRename(false),
	fMessage(nullptr),
	fTextControl(nullptr)
{
}


ProjectItem::~ProjectItem()
{
	delete fSourceItem;
	delete fTextControl;
}


void
ProjectItem::DrawItem(BView* owner, BRect bounds, bool complete)
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

	if (GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
		ProjectFolder *projectFolder = (ProjectFolder *)GetSourceItem();
		if (projectFolder->Active()) {
			owner->SetFont(be_bold_font);
		}
	}
	if (IsSelected())
		owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
	else
		owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	auto icon = IconCache::GetIcon(GetSourceItem()->Path());

	float iconSize = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
	BPoint iconStartingPoint(bounds.left + 4.0f, bounds.top  + (bounds.Height() - iconSize) / 2.0f);

	fTextRect.top = bounds.top - 0.5f;
	fTextRect.left = iconStartingPoint.x + iconSize + be_control_look->DefaultLabelSpacing();
	fTextRect.bottom = bounds.bottom-1;
	fTextRect.right = bounds.right;

	if (icon != nullptr) {
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->DrawBitmapAsync(icon, iconStartingPoint);
	}

	// Check if there is an InitRename request and show a TextControl
	if (fInitRename) {
		_DrawTextWidget(owner);
	} else {
		// Draw string at the right of the icon
		owner->SetDrawingMode(B_OP_COPY);
		owner->MovePenTo(iconStartingPoint.x + iconSize + be_control_look->DefaultLabelSpacing(),
							bounds.top + BaselineOffset());
		owner->DrawString(Text());
		owner->Sync();

		if (fFirstTimeRendered) {
			owner->Invalidate();
			fFirstTimeRendered = false;
		}
	}
}


void
ProjectItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, be_bold_font);
}


void
ProjectItem::InitRename(BMessage* message)
{
	if (fTextControl != nullptr)
		debugger("ProjectItem::InitRename() called twice!");
	fInitRename = true;
	fMessage = message;
}

void
ProjectItem::AbortRename()
{
	if (fTextControl != nullptr)
		_DestroyTextWidget();
	fInitRename = false;
}

void
ProjectItem::CommitRename()
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
ProjectItem::_DrawTextWidget(BView* owner)
{
	if (fTextControl == nullptr) {
		fTextControl = new TemporaryTextControl(fTextRect, "RenameTextWidget",
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
ProjectItem::_DestroyTextWidget()
{
	if (fTextControl != nullptr) {
		fTextControl->RemoveSelf();
		delete fTextControl;
		fTextControl = nullptr;
	}
}
