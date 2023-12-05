/*
 * Copyright 2023 Nexus6 (nexus6.haiku@icloud.com)
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StyledItem.h"

#include <Bitmap.h>
#include <ControlLook.h>
#include <Font.h>
#include <NodeInfo.h>
#include <View.h>
#include <Window.h>

#include "Utils.h"


StyledItem::StyledItem(const char* text,
						uint32 outlineLevel,
						bool expanded,
						const char *iconName)
	:
	BStringItem(text, outlineLevel, expanded),
	fToolTipText(nullptr),
	fIconName(iconName)
{
}


/* virtual */
StyledItem::~StyledItem()
{
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

	float iconSize = 0;
	BRect iconRect = bounds;
	iconRect.right = iconRect.left + 4;
	if (fIconName != nullptr) {
		iconSize = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
		BBitmap* icon = new BBitmap(BRect(iconSize - 1.0f), 0, B_RGBA32);
		GetVectorIcon(fIconName.String(), icon);
		iconRect = DrawIcon(owner, bounds, icon, iconSize);
		delete icon;
	}
	BPoint textPoint(iconRect.right + be_control_look->DefaultLabelSpacing(),
					bounds.top + BaselineOffset());
	DrawText(owner, textPoint);

	owner->Sync();
}


void
StyledItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, font);
}


bool
StyledItem::HasToolTip() const
{
	return !fToolTipText.IsEmpty();
}


void
StyledItem::SetToolTipText(const char *text)
{
	fToolTipText = text;
}


const char*
StyledItem::GetToolTipText() const
{
	return fToolTipText.String();
}

	
/* virtual */
void
StyledItem::DrawText(BView* owner, const BPoint& point)
{
	BFont font;
	owner->GetFont(&font);
	font.SetFace(B_REGULAR_FACE);
	owner->SetFont(&font);

	owner->SetDrawingMode(B_OP_COPY);
	owner->MovePenTo(point);
	owner->DrawString(Text());

	owner->Sync();
}


/* virtual */
BRect
StyledItem::DrawIcon(BView* owner, const BRect& itemBounds,
	const BBitmap* icon, float &iconSize)
{
	BPoint iconStartingPoint(itemBounds.left + 4.0f,
		itemBounds.top + (itemBounds.Height() - iconSize) / 2.0f);
	if (icon != nullptr) {
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->DrawBitmapAsync(icon, iconStartingPoint);
	}

	return BRect(iconStartingPoint, BSize(iconSize, iconSize));
}
