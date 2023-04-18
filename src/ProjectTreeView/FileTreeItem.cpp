/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>
#include <ControlLook.h>
#include <Font.h>
#include <NodeInfo.h>
#include <StringItem.h>
#include <View.h>

#include "FileTreeItem.h"
#include "IconCache.h"

FileTreeItem::FileTreeItem()
	:
	BStringItem(""),
	fIcon(nullptr),
	fFileTreeView(nullptr),
	fFirstTimeRendered(true),
	fInitStatus(B_NOT_INITIALIZED)
{
}

FileTreeItem::FileTreeItem(const entry_ref& ref, BOutlineListView *view)
	:
	BStringItem(""),
	fIcon(nullptr),
	fFileTreeView(view),
	fFirstTimeRendered(true),
	fInitStatus(B_NOT_INITIALIZED)
{
	SetTo(ref);
}

FileTreeItem::~FileTreeItem()
{
}

void
FileTreeItem::SetTo(const entry_ref& ref)
{
	fRef = ref;
	fPath = new BPath(&fRef);
	fEntry = new BEntry(&fRef);
	fStringPath = new BString(fPath->Path());
	
	BStringItem::SetText(fRef.name),
	fIcon = IconCache::GetIcon(&fRef);
	fInitStatus = B_OK;
}

void 
FileTreeItem::DrawItem(BView* owner, BRect bounds, bool complete)
{

	if (Text() == NULL)
		return;

	BFont font(be_plain_font);
	owner->SetFont(&font);
		
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
	
	float size = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
	BPoint p(bounds.left + 4.0f, bounds.top  + (bounds.Height() - size) / 2.0f);	

	owner->SetDrawingMode(B_OP_ALPHA);
	if (fIcon != nullptr)
		owner->DrawBitmapAsync(fIcon, p);
	
	// Draw string at the right of the icon
	owner->SetDrawingMode(B_OP_COPY);
	owner->MovePenTo(p.x + size + be_control_look->DefaultLabelSpacing(),
		bounds.top + BaselineOffset());
	owner->DrawString(Text());
	
	owner->Sync();
	
	if (fFirstTimeRendered) {
		owner->Invalidate();
		fFirstTimeRendered = false;
	}
}

void
FileTreeItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, be_bold_font);
}

void
FileTreeItem::SetText(const char* text)
{
	BStringItem::SetText(text);
}