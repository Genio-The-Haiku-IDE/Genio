/*
 * Copyright 2018 Kacper Kasper 
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Border drawing code lifted from kits/tracker/Navigator.cpp
 * Copyright 2015 John Scipione
 */


#include "ToolBar.h"

#include <Bitmap.h>
#include <Button.h>
#include <ControlLook.h>
#include <Handler.h>

#include "Utils.h"


ToolBar::ToolBar(BHandler* defaultTarget)
	:
	BToolBar(B_HORIZONTAL),
	fDefaultTarget(defaultTarget),
	fIconSize(24.0f)
{
	GroupLayout()->SetInsets(0.0f, 0.0f, B_USE_HALF_ITEM_INSETS, 1.0f);
		// 1px bottom inset used for border

	// Needed to draw the bottom border and to avoid a BToolBar tool-tip-hide bug
	SetFlags((Flags() | B_WILL_DRAW) & ~B_PULSE_NEEDED);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
ToolBar::Draw(BRect updateRect)
{
	// Draw a 1px bottom border, like BMenuBar
	BRect rect(Bounds());
	rgb_color base = LowColor();
	uint32 flags = 0;

	be_control_look->DrawBorder(this, rect, updateRect, base,
		B_PLAIN_BORDER, flags, BControlLook::B_BOTTOM_BORDER);

	BToolBar::Draw(rect & updateRect);
}


void
ToolBar::AddAction(uint32 command, const char* toolTipText,
	const char* iconName, bool lockable)
{
	BBitmap *icon = nullptr;
	if(iconName != nullptr) {
		icon = new BBitmap(BRect(fIconSize - 1.0f), 0, B_RGBA32);
		GetVectorIcon(iconName, icon);
		fActionIcons.emplace(std::make_pair(command, iconName));
	}
	BToolBar::AddAction(command, fDefaultTarget, icon, toolTipText,
		nullptr, lockable);
}


void
ToolBar::ChangeIconSize(float newSize)
{
	fIconSize = newSize;
	BBitmap icon(BRect(fIconSize - 1.0f), 0, B_RGBA32);
	for(const auto& action : fActionIcons) {
		GetVectorIcon(action.second.c_str(), &icon);
		BToolBar::FindButton(action.first)->SetIcon(&icon);
	}
}
