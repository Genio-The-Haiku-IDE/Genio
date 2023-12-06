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
	fIconName(iconName),
	fFontFace(B_REGULAR_FACE),
	fToolTipText()
{
}


/* virtual */
StyledItem::~StyledItem()
{
}


/* virtual */
void
StyledItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
	// TODO: Inherited classes which reimplement this method could duplicate
	// most of this code. See if there's a way to move it to a common method
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

	if (IsSelected())
		owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
	else
		owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	// TODO: until here (see comment above)

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

	// TODO: would be nice to draw extra text in different style
	BString text(Text());
	text << fExtraText;
	DrawText(owner, text.String(), textPoint);

	owner->Sync();
}


/* virtual */
void
StyledItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, font);
}


void
StyledItem::SetTextFontFace(uint16 fontFace)
{
	fFontFace = fontFace;
}


void
StyledItem::SetExtraText(const char* extraText)
{
	fExtraText = extraText;
}


const char*
StyledItem::ExtraText() const
{
	return fExtraText.String();
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
StyledItem::DrawText(BView* owner, const char* text, const BPoint& point)
{
	BFont font;
	owner->GetFont(&font);
	font.SetFace(fFontFace);
	owner->SetFont(&font);

	owner->SetDrawingMode(B_OP_COPY);
	owner->MovePenTo(point);
	owner->DrawString(text);

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
