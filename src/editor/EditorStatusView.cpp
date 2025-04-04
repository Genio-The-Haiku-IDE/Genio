/*
 * Copyright 2002-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vlad Slepukhin
 *		Siarzhuk Zharski
 *
 * Copied from Haiku commit a609673ce8c942d91e14f24d1d8832951ab27964.
 * Modifications:
 * Copyright 2018-2019 Kacper Kasper <kacperkasper@gmail.com>
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * Distributed under the terms of the MIT License.
 */


#include "EditorStatusView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Catalog.h>
#include <ControlLook.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Window.h>

#include "ActionManager.h"
#include "GenioWindowMessages.h"
#include "Editor.h"

const float kHorzSpacing = 5.f;

using namespace BPrivate;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "EditorStatusView"

#define CHECKMARK_ICON "\xE2\x9C\x94"
#define ESCLAMATIONMARK_ICON "\xE2\x9D\x97"

namespace editor {

BPopUpMenu* StatusView::sMenu = nullptr;

StatusView::StatusView(Editor* editor)	:
			controls::StatusView(dynamic_cast<BScrollView*>(editor)),
			fNavigationPressed(false),
			fNavigationButtonWidth(B_H_SCROLL_BAR_HEIGHT)
{
	BScrollBar* scrollBar = ScrollView()->ScrollBar(B_HORIZONTAL);
	if (scrollBar != NULL)
		fNavigationButtonWidth = scrollBar->Bounds().Height() + 1;

	memset(fCellWidth, 0, sizeof(fCellWidth));

	SetFont(be_fixed_font);
	SetFontSize(12.);
}


StatusView::~StatusView()
{
}


void
StatusView::AttachedToWindow()
{
	BMessage message(UPDATE_STATUS);
	message.AddInt32("line", 1);
	message.AddInt32("column", 1);

	SetStatus(&message);
	controls::StatusView::AttachedToWindow();
}


void
StatusView::Draw(BRect updateRect)
{
	float width, height;
	GetPreferredSize(&width, &height);
	if (width <= 0)
		return;

	BRect bounds(Bounds());
	bounds.bottom = height;
	bounds.right = width;
	uint32 flags = 0;
	if (!Window()->IsActive())
		flags |= BControlLook::B_DISABLED;
	be_control_look->DrawScrollBarBackground(this, bounds, bounds, ViewColor(),
		flags, B_HORIZONTAL);
	// DrawScrollBarBackground mutates the rect
	bounds.left = 0.0f;

	SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
	StrokeLine(bounds.LeftTop(), bounds.RightTop());

	// Navigation button
	BRect navRect(bounds);
	navRect.left--;
	navRect.right = fNavigationButtonWidth + 1;
	StrokeLine(navRect.RightTop(), navRect.RightBottom());
	_DrawNavigationButton(navRect);
	bounds.left = navRect.right + 1;

	// BControlLook mutates color
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));

	float x = bounds.left;
	for (size_t i = 0; i < kStatusCellCount - 1; i++) {
		if (fCellText[i].IsEmpty())
			continue;
		x += fCellWidth[i];
		if (fCellText[i + 1].IsEmpty() == false)
			StrokeLine(BPoint(x, bounds.top + 3), BPoint(x, bounds.bottom - 3));
	}

	SetLowColor(ViewColor());
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	x = bounds.left;
	float y = (bounds.bottom + bounds.top
		+ ceilf(fontHeight.ascent) - ceilf(fontHeight.descent)) / 2;

	for (size_t i = 0; i < kStatusCellCount; i++) {
		if (fCellText[i].Length() == 0)
			continue;
		DrawString(fCellText[i], BPoint(x + kHorzSpacing, y));
		x += fCellWidth[i];
	}
}


void
StatusView::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	if ((transit == B_ENTERED_VIEW) || (transit == B_INSIDE_VIEW)) {
		where.y = 0;
		if (_GetCellRect(kEditorConfig).Contains(where)) {
			if (fCellText[kEditorConfig] == BString(EDITORCONFIG_YES_GLYPH))
				SetToolTip(B_TRANSLATE("Settings in .editorconfig applied"));
			else
				SetToolTip(B_TRANSLATE("Global settings applied"));
		} else if (_GetCellRect(kStatus).Contains(where)){
			if (fStatusDescription.IsEmpty())
				SetToolTip(B_TRANSLATE("No LSP file status information"));
			else
				SetToolTip(fStatusDescription.String()); //FIX: translation??
		} else {
			SetToolTip("");
		}
	}
}


BRect
StatusView::_GetCellRect(int32 cell)
{
	BRect rect(Bounds());
	rect.left = 0;

	for (int32 i = 0; i < cell; i++) {
		rect.left += fCellWidth[i];
	}
	rect.left += fNavigationButtonWidth;
	rect.right = rect.left + fCellWidth[cell];

	return rect;
}


BString
StatusView::_FromStatusToIcon(BString status)
{
	if (status.IsEmpty())
		return " "; //TODO: better symbol?
	if (status == "idle") //clangd specific.. (TODO: how to make it generic without a standard?)
		return CHECKMARK_ICON;

	return ESCLAMATIONMARK_ICON;
}



void
StatusView::MouseDown(BPoint where)
{
	if (where.x < fNavigationButtonWidth) {
		fNavigationPressed = true;
		Invalidate();
		_ShowDirMenu();
		return;
	}
	where.y = 0;
	if (_GetCellRect(kPositionCell).Contains(where)) {
		BMessenger msgr(Window());
		msgr.SendMessage(MSG_GOTO_LINE);
	} else if (_GetCellRect(kOverwriteMode).Contains(where)) {

		BMessenger msgr(Window());
		msgr.SendMessage(MSG_TEXT_OVERWRITE);
	}
}


float
StatusView::Width()
{
	float width = fNavigationButtonWidth;
	for (size_t i = 0; i < kStatusCellCount; i++) {
		if (fCellText[i].Length() == 0) {
			fCellWidth[i] = 0;
			continue;
		}
		float cellWidth = ceilf(StringWidth(fCellText[i]));
		if (cellWidth > 0)
			cellWidth += kHorzSpacing * 2;
		if (cellWidth > fCellWidth[i] || i != kPositionCell)
			fCellWidth[i] = cellWidth;
		width += fCellWidth[i];
	}
	return width;
}


void
StatusView::SetStatus(BMessage* message)
{
	int32 line = 0, column = 0;
	if (message->FindInt32("line", &line) == B_OK
		&& message->FindInt32("column", &column) == B_OK) {
		fCellText[kPositionCell].SetToFormat("%" B_PRIi32 ":%" B_PRIi32, line, column);
	}

	fStatusDescription = message->GetString("status", "");
	fCellText[kStatus] = _FromStatusToIcon(fStatusDescription);
	fCellText[kOverwriteMode] 	= message->GetString("overwrite", "");
	fCellText[kLineFeed] 	  	= message->GetString("eol", "");
	fCellText[kFileStateCell] 	= message->GetString("readOnly", "");
	fCellText[kEditorConfig] 	= message->GetString("editorconfig", "");
	fCellText[kTrimWhitespace] 	= message->GetString("trim_trialing_whitespace", "");

	BString style = message->GetString("indent_style", "");
	int32 size = message->GetInt32("indent_size", -1);

	fCellText[kIndent].SetToFormat("%s:%" B_PRIi32, style.String(), size);

	Invalidate();
}


void
StatusView::_DrawNavigationButton(BRect rect)
{
	rect.InsetBy(1.0f, 1.0f);
	rgb_color baseColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_LIGHTEN_1_TINT);
	uint32 flags = 0;
	if (fNavigationPressed)
		flags |= BControlLook::B_ACTIVATED;
	if (Window()->IsActive() == false)
		flags |= BControlLook::B_DISABLED;
	be_control_look->DrawButtonBackground(this, rect, rect, baseColor, flags,
		BControlLook::B_ALL_BORDERS, B_HORIZONTAL);
	rect.InsetBy(0.0f, -1.0f);
	be_control_look->DrawArrowShape(this, rect, rect, baseColor,
		BControlLook::B_DOWN_ARROW, flags, B_DARKEN_MAX_TINT);
}


void
StatusView::_CreateMenu(BWindow* window)
{
	sMenu = new BPopUpMenu("EditorMenu");
	ActionManager::AddItem(MSG_BUFFER_LOCK, sMenu);
	sMenu->SetTargetForItems(window);
}


void
StatusView::_ShowDirMenu()
{
	if (!sMenu)
		StatusView::_CreateMenu(Window());

	BPoint point = Parent()->Bounds().LeftBottom();
	point.y += 3 + B_H_SCROLL_BAR_HEIGHT;
	ConvertToScreen(&point);
	BRect clickToOpenRect(Parent()->Bounds());
	ConvertToScreen(&clickToOpenRect);
	StatusView::sMenu->Go(point, true, true, clickToOpenRect);
	fNavigationPressed = false;
}

} // namespace editor
