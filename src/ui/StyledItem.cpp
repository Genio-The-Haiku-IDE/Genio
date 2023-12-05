/*
 * Copyright 2023 Nexus6 ()
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
	fMessage(nullptr),
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

	BPoint textPoint(iconStartingPoint.x + iconSize + be_control_look->DefaultLabelSpacing(),
					bounds.top + BaselineOffset());
	_DrawText(owner, textPoint);

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

	
void
StyledItem::_DrawText(BView* owner, const BPoint& point)
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