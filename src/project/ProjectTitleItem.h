/*
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * ProjectTitleItem class taken from Preferences/Network/InterfaceListItem.cpp
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROJECT_TITLE_ITEM_H
#define PROJECT_TITLE_ITEM_H

#include <OutlineListView.h>

class ProjectTitleItem : public BStringItem {
public:
	ProjectTitleItem(const char* title, bool active)
		:
		BStringItem(title)
		, fActive(active)
	{
	}

	void DrawItem(BView* owner, BRect bounds, bool complete)
	{
		BFont font;

		font.SetFamilyAndStyle("Noto Sans", "Bold");
		owner->SetFont(&font);

		if (IsSelected())
			owner->SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
		else
			owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	if (fActive == false) {
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
//		owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
		owner->SetHighColor(0, 0, 0, 128);
	} else {
		owner->SetDrawingMode(B_OP_OVER);
		owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
	}


		BStringItem::DrawItem(owner, bounds, complete);
		owner->SetFont(be_plain_font);
	}

	void Update(BView* owner, const BFont* font)
	{
		BStringItem::Update(owner, be_bold_font);
	}

	void Activate()
	{
		fActive = true;
	}

	void Deactivate()
	{
		fActive = false;
	}
	
private:
	bool fActive;
};

#endif // PROJECT_TITLE_ITEM_H
