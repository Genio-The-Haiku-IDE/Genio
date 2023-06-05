/*
 * TitleItem class taken from Preferences/Network
 * Copyright 2004-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *	Authors:
 *		Adrien Destugues, <pulkomandy@pulkomandy.tk>
 *		Axel Dörfler, <axeld@pinc-software.de>
 *		Alexander von Gluck, <kallisti5@unixzen.com>
 */
#ifndef TITLE_ITEM_H
#define TITLE_ITEM_H

class TitleItem : public BStringItem {
public:
	TitleItem(BString title)
		:
		BStringItem(title)
	{
	}

	void DrawItem(BView* owner, BRect bounds, bool complete)
	{
		owner->PushState();

		owner->SetFont(be_bold_font);
		BStringItem::DrawItem(owner, bounds, complete);

		owner->PopState();
	}

	void Update(BView* owner, const BFont* font)
	{
		BStringItem::Update(owner, be_bold_font);
	}
};

#endif // TITLE_ITEM_H
