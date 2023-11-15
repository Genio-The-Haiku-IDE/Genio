/*
 * Copyright 2023 Nexus6 (nexus6.haiku@icloud.com)
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <Bitmap.h>
#include <Font.h>
#include <OutlineListView.h>
#include <Messenger.h>
#include <StringItem.h>
#include <TextControl.h>
#include <View.h>

class StyledItem : public BStringItem {
public:
					StyledItem(BOutlineListView *container,
								const char* text,
								uint32 outlineLevel = 0,
								bool expanded = true,
								const char *iconName = nullptr);
					~StyledItem();

	void 			DrawItem(BView* owner, BRect bounds, bool complete);
	void 			Update(BView* owner, const BFont* font);

	void			SetPrimaryTextStyle(uint16 face);
	void			SetSecondaryTextStyle(uint16 face);

	void			InitRename(BMessage* message);
	void			AbortRename();
	void			CommitRename();

	bool			HasToolTip() { return !fToolTipText.IsEmpty(); };
	void			SetToolTipText(const char *text) { fToolTipText = text; }
	const char*		GetToolTipText() { return fToolTipText.String(); }

private:
	BOutlineListView* fContainerListView;
	bool			fFirstTimeRendered;
	bool			fInitRename;
	BMessage*		fMessage;
	BTextControl	*fTextControl;
	BString			fPrimaryText;
	BString			fSecondaryText;
	BString			fToolTipText;
	uint16			fPrimaryTextStyle;
	uint16			fSecondaryTextStyle;
	BString			fIconName;

	void			_DrawText(BView* owner, const BPoint& textPoint);
	void			_DrawTextWidget(BView* owner, const BRect& textRect);
	void			_DestroyTextWidget();
};