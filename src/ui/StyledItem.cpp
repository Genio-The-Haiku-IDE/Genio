/*
 * Copyright 2023 Nexus6 ()
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StyledItem.h"


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
	fFontFace(B_REGULAR_FACE),
	fExtraTextColor(nullptr),
	fToolTipText()
{
}


/* virtual */
StyledItem::~StyledItem()
{
	delete fExtraTextColor;
}


/* virtual */
void
StyledItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
	DrawItemPrepare(owner, bounds, complete);

	float iconSize = be_control_look->ComposeIconSize(B_MINI_ICON).Height();
	BRect iconRect = bounds;
	iconRect.right = iconRect.left + 4;
	iconRect = DrawIcon(owner, bounds, iconSize);
	BPoint textPoint(iconRect.right + be_control_look->DefaultLabelSpacing(),
					bounds.top + BaselineOffset());

	// TODO: would be nice to draw extra text in different style
	// at least we can draw in different color now
	DrawText(owner, Text(), ExtraText(), textPoint);

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


uint16
StyledItem::TextFontFace() const
{
	return fFontFace;
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


void
StyledItem::SetExtraTextColor(const rgb_color& color)
{
	delete fExtraTextColor;
	fExtraTextColor = new rgb_color(color);
}


rgb_color
StyledItem::ExtraTextColor() const
{
	if (fExtraTextColor != nullptr)
		return *fExtraTextColor;

	// TODO: might not be what we want
	if (IsSelected())
		return ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	else
		return ui_color(B_LIST_ITEM_TEXT_COLOR);
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
StyledItem::DrawItemPrepare(BView* owner, BRect bounds, bool complete)
{
	if (Text() == NULL)
		return;

	if (IsSelected() || complete) {
		const rgb_color oldLowColor = owner->LowColor();
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
}


/* virtual */
void
StyledItem::DrawText(BView* owner, const char* text,
				const char* extraText, const BPoint& point)
{
	BFont font;
	font.SetFace(fFontFace);
	owner->SetFont(&font, B_FONT_FACE);

	owner->SetDrawingMode(B_OP_COPY);
	owner->MovePenTo(point);
	owner->DrawString(text);
	if (extraText != nullptr) {
		owner->SetHighColor(ExtraTextColor());
		owner->DrawString(extraText);
	}
}


/* virtual */
BRect
StyledItem::DrawIcon(BView* owner, const BRect& itemBounds,
					const float & /*iconSize*/)
{
	BRect bounds = itemBounds;
	bounds.right = bounds.left;
	return bounds;
}
