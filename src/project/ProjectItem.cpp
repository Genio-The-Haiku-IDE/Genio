/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Bitmap.h>
#include <ControlLook.h>
#include <Font.h>
#include <NodeInfo.h>
#include <StringItem.h>
#include <View.h>

#include "ProjectFolder.h"
#include "ProjectItem.h"
#include "IconCache.h"

ProjectItem::ProjectItem(SourceItem *sourceItem)
	:
	BStringItem(sourceItem->Name())
{
	fSourceItem = sourceItem;
}

ProjectItem::~ProjectItem()
{
	delete fSourceItem;
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
	
	if (GetSourceItem()->Type() == SourceItemType::ProjectFolderItem)
	{
		ProjectFolder *projectFolder = (ProjectFolder *)GetSourceItem();
		if (projectFolder->Active())
		{
			owner->SetDrawingMode(B_OP_OVER);
			owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
			owner->SetFont(be_bold_font);
		} else {
			owner->SetDrawingMode(B_OP_OVER);
			owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
			owner->SetFont(be_plain_font);
		}
	} else {
		owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
	}

	owner->SetDrawingMode(B_OP_ALPHA);
	BEntry entry(GetSourceItem()->Path());
	entry_ref ref;
	entry.GetRef(&ref);
	auto icon = IconCache::GetIcon(&ref);
	
	float size = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
	BPoint p(bounds.left + 4.0f, bounds.top  + (bounds.Height() - size) / 2.0f);	
	
	if (icon!=nullptr)
		owner->DrawBitmapAsync(icon, p);
	
	// Draw string at the right of the icon
	owner->SetDrawingMode(B_OP_COPY);
	owner->MovePenTo(p.x + size + be_control_look->DefaultLabelSpacing(),
		bounds.top + BaselineOffset());
	owner->DrawString(Text());
	
	owner->Sync();
	
	if (firstTimeRendered) {
		owner->Invalidate();
		firstTimeRendered = false;
	}
}

void
ProjectItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, be_bold_font);
}

