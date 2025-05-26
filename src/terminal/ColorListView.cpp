/*
 * Copyright 2016-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		John Scipione, jscipione@gmail.com
 */


#include "ColorListView.h"

#include <algorithm>

#include <math.h>
#include <stdio.h>

#include <Bitmap.h>
#include "ColorItem.h"
#include <String.h>


// golden ratio
#ifdef M_PHI
#	undef M_PHI
#endif
#define M_PHI 1.61803398874989484820


namespace BPrivate {


//	#pragma mark - ColorListView


BColorListView::BColorListView(const char* name, list_view_type type, uint32 flags)
	:
	BListView(name, type, flags)
{
}


BColorListView::~BColorListView()
{
}


bool
BColorListView::InitiateDrag(BPoint where, int32 index, bool wasSelected)
{
	BColorItem* item = dynamic_cast<BColorItem*>(ItemAt(index));
	if (item == NULL)
		return false;

	rgb_color color = item->Color();

	BString hexStr;
	hexStr.SetToFormat("#%.2X%.2X%.2X", color.red, color.green, color.blue);

	BMessage message(B_PASTE);
	message.AddData("text/plain", B_MIME_TYPE, hexStr.String(), hexStr.Length());
	message.AddData("RGBColor", B_RGB_COLOR_TYPE, &color, sizeof(color));
	message.AddMessenger("be:sender", BMessenger(this));
	message.AddPointer("source", this);
	message.AddInt64("when", (int64)system_time());

	float itemHeight = item->Height() - 5;
	BRect rect(0, 0, roundf(itemHeight * M_PHI) - 1, itemHeight - 1);

	BBitmap* bitmap = new BBitmap(rect, B_RGB32, true);
	if (bitmap->Lock()) {
		BView* view = new BView(rect, "", B_FOLLOW_NONE, B_WILL_DRAW);
		bitmap->AddChild(view);

		view->SetHighColor(B_TRANSPARENT_COLOR);
		view->FillRect(view->Bounds());

		++rect.top;
		++rect.left;

		view->SetHighColor(0, 0, 0, 100);
		view->FillRect(rect);
		rect.OffsetBy(-1, -1);

		int32 red = (int32)std::min(255, (int)(1.2 * color.red + 40));
		int32 green = (int32)std::min(255, (int)(1.2 * color.green + 40));
		int32 blue = (int32)std::min(255, (int)(1.2 * color.blue + 40));

		view->SetHighColor(red, green, blue);
		view->StrokeRect(rect);

		++rect.left;
		++rect.top;

		red = (int32)(0.8 * color.red);
		green = (int32)(0.8 * color.green);
		blue = (int32)(0.8 * color.blue);

		view->SetHighColor(red, green, blue);
		view->StrokeRect(rect);

		--rect.right;
		--rect.bottom;

		view->SetHighColor(color.red, color.green, color.blue);
		view->FillRect(rect);
		view->Sync();

		bitmap->Unlock();
	}

	DragMessage(&message, bitmap, B_OP_ALPHA, BPoint(14, 14));

	return true;
}


void
BColorListView::MessageReceived(BMessage* message)
{
	// if we received a dropped message, see if it contains color data
	if (!message->WasDropped())
		return BListView::MessageReceived(message);
		
	BPoint dropPoint = message->DropPoint();
	ConvertFromScreen(&dropPoint);
	int32 index = IndexOf(dropPoint);
	BColorItem* item = dynamic_cast<BColorItem*>(ItemAt(index));
	if (item == NULL)
		return BListView::MessageReceived(message);

	char* name;
	type_code type;
	rgb_color* color;
	ssize_t size;
	if (message->GetInfo(B_RGB_COLOR_TYPE, 0, &name, &type) == B_OK
		&& message->FindData(name, type, (const void**)&color, &size) == B_OK) {
		// set message command to set current color or new
		bool current = index == CurrentSelection();
		uint32 command = (current ? BColorListView::B_MESSAGE_SET_CURRENT_COLOR
			: BColorListView::B_MESSAGE_SET_COLOR);
		message->what = command;

		// if setting different color, add which color to set
		if (current)
			message->AddBool("current", true);
		else
			message->AddUInt32("which", (uint32)item->ColorWhich());

		// build messenger and send message
		BMessenger messenger = BMessenger(Parent());
		if (messenger.IsValid())
			messenger.SendMessage(message);

		// set current color redraws for us
		if (!current) {
			item->SetColor(*color);
			InvalidateItem(index);
		}
	}
}


} // namespace BPrivate
