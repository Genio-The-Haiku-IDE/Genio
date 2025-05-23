/*
 * Copyright 2023 Nexus6 (nexus6.haiku@icloud.com)
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <StringItem.h>

#include <Font.h>

class StyledItem : public BStringItem {
public:

					StyledItem(const char* text,
								uint32 outlineLevel = 0,
								bool expanded = true);
					virtual ~StyledItem();

	virtual void 	DrawItem(BView* owner, BRect bounds, bool complete);
	virtual void 	Update(BView* owner, const BFont* font);

	// TODO: Maybe SetFont ?
	void			SetTextFontFace(uint16 fontFace);
	uint16			TextFontFace() const;

	void			SetExtraText(const char* extraText);
	const char*		ExtraText() const;
	void			SetExtraTextColor(const rgb_color& color);
	rgb_color		ExtraTextColor() const;

	bool			HasToolTip() const;
	void			SetToolTipText(const char *text);
	const char*		GetToolTipText() const;

protected:
	// called from DrawItem: sets up colors
	// can be called by overridden DrawItem() to avoid duplication
	virtual void	DrawItemPrepare(BView* owner, BRect bounds, bool complete);

	virtual BRect	DrawIcon(BView* owner, const BRect& bounds,
							const float& iconSize);
	virtual void	DrawText(BView* owner, const char* text,
						const char* extraText, const BPoint& textPoint);
private:
	uint16			fFontFace;
	// TODO: Maybe allocate these dynamically since ExtraText
	// is not always used
	BString			fExtraText;
	rgb_color* 		fExtraTextColor;
	BString			fToolTipText;
};
