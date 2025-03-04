/*
 * Copyright 2025, Andrea Anzani 
 * Original source code by:
 * Copyright (C) 2010 Rene Gollent 
 * Copyright (C) 2010 Stephan Aßmus 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <Button.h>
#include <ControlLook.h>
#include <Window.h>


class TabViewTools {
	public:
		static float DefaultTabHeight() {
			static float _heigh = -1.0;
			if (_heigh == -1) {
				font_height fh;
				be_plain_font->GetHeight(&fh);
				_heigh = ceilf(fh.ascent + fh.descent + fh.leading +
					(be_control_look->DefaultLabelSpacing() * 1.3f));
			}
			return _heigh;
		}

		static float DefaultFontDescent() {
			static float _desc = -1.0;
			if (_desc == -1) {
				font_height fh;
				be_plain_font->GetHeight(&fh);
				_desc = fh.descent;
			}
			return _desc;
		}

		static void DrawTabBackground(BView* view, BRect& bounds, BRect& updateRect) {
			rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
			uint32 borders = BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER;
			be_control_look->DrawInactiveTab(view, bounds, updateRect, base, 0, borders);
		}

		static void DrawDroppingZone(BView* view, BRect rect)
		{
			rgb_color color = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
			color.alpha = 170;
			view->SetHighColor(color);
			view->SetDrawingMode(B_OP_ALPHA);
			view->FillRect(rect);
		}
};


class GTabButton : public BButton {
public:
	GTabButton(const char* label = nullptr, BMessage* message = nullptr)
		: BButton(label, message)
	{
		SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
		SetExplicitMaxSize(BSize(50, B_SIZE_UNSET));
		SetExplicitMinSize(BSize(50, B_SIZE_UNSET));
		SetExplicitPreferredSize(BSize(50, B_SIZE_UNSET));
	}

	virtual BSize MinSize()
	{
		return BSize(20, TabViewTools::DefaultTabHeight());
	}

	virtual BSize MaxSize()
	{
		return MinSize();
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds(Bounds());
		TabViewTools::DrawTabBackground(this, bounds, updateRect);

		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		if (IsEnabled()) {
			uint32 flags = be_control_look->Flags(this);
			rgb_color button = tint_color(base, 1.07);
			be_control_look->DrawButtonBackground(this, bounds, updateRect,
				button, flags, 0);
		}
		bounds.left = (bounds.left + bounds.right) / 2 - 6;
		bounds.top = (bounds.top + bounds.bottom) / 2 - 6;
		bounds.right = bounds.left + 12;
		bounds.bottom = bounds.top + 12;
		DrawSymbol(bounds, updateRect, base);
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
	}
};


class GTabMenuTabButton : public GTabButton {
public:
	GTabMenuTabButton(BMessage* message)
		:
		GTabButton(" ", message),
		fCloseTime(0)
	{
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		float tint = IsEnabled() ? B_DARKEN_4_TINT : B_DARKEN_1_TINT;
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_DOWN_ARROW, 0, tint);
	}

	virtual void MouseDown(BPoint point)
	{
		// Don't reopen the menu if it's already open or freshly closed.
		bigtime_t clickSpeed = 2000000;
		get_click_speed(&clickSpeed);
		bigtime_t clickTime = Window()->CurrentMessage()->FindInt64("when");
		if (!IsEnabled() || (Value() == B_CONTROL_ON)
			|| clickTime < fCloseTime + clickSpeed) {
			return;
		}

		// Invoke must be called before setting B_CONTROL_ON
		// for the button to stay "down"
		Invoke();
		SetValue(B_CONTROL_ON);
	}

	virtual void MouseUp(BPoint point)
	{
		// Do nothing
	}

	void MenuClosed()
	{
		fCloseTime = system_time();
		SetValue(B_CONTROL_OFF);
	}

private:
	bigtime_t fCloseTime;
};
