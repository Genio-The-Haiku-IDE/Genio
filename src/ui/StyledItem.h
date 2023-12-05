/*
 * Copyright 2023 Nexus6 ()
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <StringItem.h>

#include <Font.h>

class StyledItem : public BStringItem {
public:
					StyledItem(const char* text,
								uint32 outlineLevel = 0,
								bool expanded = true,
								const char *iconName = nullptr);
					~StyledItem();

	void 			DrawItem(BView* owner, BRect bounds, bool complete);
	void 			Update(BView* owner, const BFont* font);

	void			SetTextStyle(font_style fontStyle);
	
	bool			HasToolTip() const;
	void			SetToolTipText(const char *text);
	const char*		GetToolTipText() const;

private:
	BString			fToolTipText;
	BString			fIconName;
	font_style		fFontStyle;
	
	void			_DrawText(BView* owner, const BPoint& textPoint);
};
