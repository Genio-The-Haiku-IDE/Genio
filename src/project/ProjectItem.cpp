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
							BMessage* message, ProjectItem *item, uint32 resizingMode=B_FOLLOW_LEFT|B_FOLLOW_TOP,
							uint32 flags=B_WILL_DRAW|B_NAVIGABLE)
		:
		BTextControl(frame,  name, label, text, message, resizingMode, flags),
		fProjectItem(item)
	{
		SetEventMask(B_POINTER_EVENTS|B_KEYBOARD_EVENTS);	
	}
	
	virtual void AllAttached() {
		TextView()->SelectAll();
	}
	
	virtual void MouseDown(BPoint point) {
		if(Bounds().Contains(point))
			_inherited::MouseDown(point);
		else {
			fProjectItem->AbortRename();
		}
		
		Invoke();
		
	}
	
	virtual void KeyDown(const char* bytes, int32 numBytes) {
		if(numBytes == 1 && *bytes == B_ESCAPE) {
			fProjectItem->AbortRename();
		}
		if(numBytes == 1 && *bytes == B_RETURN) {
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

	owner->SetFont(be_plain_font);
		
	rgb_color lowColor = owner->LowColor();

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			color = owner->ViewColor();

		owner->SetLowColor(color);
		owner->FillRect(bounds, B_SOLID_LOW);
	} else
		owner->SetLowColor(owner->ViewColor());

	owner->SetLowColor(lowColor);

	if (GetSourceItem()->Type() == SourceItemType::ProjectFolderItem) {
		ProjectFolder *projectFolder = (ProjectFolder *)GetSourceItem();
		if (projectFolder->Active()) {
			owner->SetFont(be_bold_font);
		} else {
			owner->SetFont(be_plain_font);
		}
		if (IsSelected()) {
			owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
		} else {
			owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
		}
		owner->SetDrawingMode(B_OP_OVER);
	} else {
		if (IsSelected())
			owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
		else
			owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
	}

	owner->SetDrawingMode(B_OP_ALPHA);
	auto icon = IconCache::GetIcon(GetSourceItem()->Path());

	float size = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
	BPoint p(bounds.left + 4.0f, bounds.top  + (bounds.Height() - size) / 2.0f);	
	
	if (icon != nullptr)
		owner->DrawBitmapAsync(icon, p);

	// Draw string at the right of the icon
	owner->SetDrawingMode(B_OP_COPY);
	owner->MovePenTo(p.x + size + be_control_look->DefaultLabelSpacing(),
		bounds.top + BaselineOffset());
	owner->DrawString(Text());

	fTextRect.top = bounds.top;
	fTextRect.left = p.x + size + be_control_look->DefaultLabelSpacing();
	fTextRect.bottom = bounds.bottom;
	fTextRect.right = bounds.right;

	owner->Sync();
	
	if (fFirstTimeRendered) {
		owner->Invalidate();
		fFirstTimeRendered = false;
	}
}


void
ProjectItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, be_bold_font);
}


void
ProjectItem::InitRename(BMessage* message, const BMessenger& target)
{
	fInitRename = true;
	fTarget = target;
	fMessage = message;
}

void
ProjectItem::AbortRename()
{
	_DestroyTextWidget();
	fInitRename = false;
}

void
ProjectItem::CommitRename()
{
	fMessage->AddString("_value", fTextControl->Text());
	BMessenger(fTextControl->Parent()).SendMessage(fMessage);
	SetText(fTextControl->Text());
	_DestroyTextWidget();
	fInitRename = false;
}

void
ProjectItem::_DestroyTextWidget()
{
	if (fTextControl != nullptr) {
		fTextControl->RemoveSelf();
		delete fTextControl;
	}
}

