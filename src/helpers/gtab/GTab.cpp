/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GTab.h"

#include <Bitmap.h>
#include <ControlLook.h>

#include "TabsContainer.h"


#define TAB_DRAG	'DRAG'
#define ALPHA		200


bool
GTabDropZone::_ValidDragAndDrop(const BMessage* message)
{
	GTab* fromTab = (GTab*)message->GetPointer("tab", nullptr);
	TabsContainer*	fromContainer = fromTab->Container();

	if (fromTab == nullptr || fromContainer == nullptr)
		return false;

	if (this == fromTab)
		return false;

	if (Container()->GetAffinity() == 0 || fromContainer->GetAffinity() == 0)
		return false;

	return Container()->GetAffinity() == fromContainer->GetAffinity();
}


bool
GTabDropZone::DropZoneMouseMoved(BView* view,
								 BPoint where,
								 uint32 transit,
								 const BMessage* dragMessage)
{
	if (dragMessage &&
	    dragMessage->what == TAB_DRAG &&
		_ValidDragAndDrop(dragMessage)) {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW:
			{
				StartDragging(view);
				return true;
			}
			default:
				StopDragging(view);
				break;
		}
	} else {
		OnMouseMoved(where);
		StopDragging(view);
	}
	return false;
}


bool
GTabDropZone::DropZoneMessageReceived(BMessage* message)
{
	if (message->what == TAB_DRAG) {
		if(_ValidDragAndDrop(message))
			OnDropMessage(message);
		return true;
	}
	return false;
}



// GTab
GTab::GTab(const char* label)
	:
	BView("_tabView_", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fIsFront(false),
	fLabel(label)
{
}


GTab::~GTab()
{
}


BSize
GTab::MinSize()
{
	BSize size(MaxSize());
	size.width = 100.0f;
	return size;
}


BSize
GTab::MaxSize()
{
	float labelWidth = 150.0f;
	return BSize(labelWidth, TabViewTools::DefaultTabHeight());
}


void
GTab::Draw(BRect updateRect)
{
	DrawTab(this, updateRect);
	DropZoneDraw(this, Bounds());
}


void
GTab::DrawTab(BView* owner, BRect updateRect)
{
	BRect frame(owner->Bounds());
    if (fIsFront) {
        frame.left--;
        frame.right++;
	}

	DrawBackground(owner, frame, updateRect, fIsFront);

	if (fIsFront) {
		frame.top += 0.0f;
	} else
		frame.top += 3.0f;

	float spacing = be_control_look->DefaultLabelSpacing();
	frame.InsetBy(spacing, spacing / 2);
	DrawContents(owner, frame, updateRect, fIsFront);
}


void
GTab::DrawBackground(BView* owner, BRect frame, const BRect& updateRect, bool isFront)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 borders = BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER;

	if (isFront) {
		be_control_look->DrawActiveTab(owner, frame, updateRect, base,
			0, borders);
	} else {
		be_control_look->DrawInactiveTab(owner, frame, updateRect, base,
			0, borders);
	}
}


void
GTab::DrawContents(BView* owner, BRect frame, const BRect& updateRect, bool isFront)
{
	DrawLabel(owner, frame, updateRect, isFront);
}


void
GTab::DrawLabel(BView* owner, BRect frame, const BRect& updateRect, bool isFront)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	be_control_look->DrawLabel(owner, fLabel.String(), frame, updateRect,
		base, 0, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE));
}


void
GTab::MouseDown(BPoint where)
{
	BMessage* msg = Window()->CurrentMessage();
	if (msg == nullptr)
		return;

	const int32 buttons = msg->GetInt32("buttons", 0);

 	if (Container())
		Container()->MouseDownOnTab(this, where, buttons);

	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		DropZoneMouseDown(where);
	}
}


void
GTab::MouseUp(BPoint where)
{
	DropZoneMouseUp(this, where);
}


void
GTab::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	DropZoneMouseMoved(this, where, transit, dragMessage);
}


void
GTab::MessageReceived(BMessage* message)
{
	if (DropZoneMessageReceived(message) == false)
		BView::MessageReceived(message);
}


bool
GTab::InitiateDrag(BPoint where)
{
	GTab* tab = this;
	if (tab != nullptr) {
		BMessage message(TAB_DRAG);

		message.AddPointer("tab", this);
		message.AddPointer("tab_container", Container());

		const BRect& updateRect = tab->Bounds();

		BBitmap* dragBitmap = new BBitmap(updateRect, B_RGB32, true);
		if (dragBitmap->IsValid()) {
			BView* view = new BView(dragBitmap->Bounds(), "helper", B_FOLLOW_NONE, B_WILL_DRAW);
			dragBitmap->AddChild(view);
			dragBitmap->Lock();
			tab->DrawTab(view, updateRect);
			view->Sync();
			uint8* bits = reinterpret_cast<uint8*>(dragBitmap->Bits());
			int32 height = dragBitmap->Bounds().IntegerHeight() + 1;
			int32 width = dragBitmap->Bounds().IntegerWidth() + 1;
			int32 bpr = dragBitmap->BytesPerRow();
			for (int32 y = 0; y < height; y++, bits += bpr) {
				uint8* line = bits + 3;
				for (uint8* end = line + 4 * width; line < end; line += 4)
					*line = ALPHA;
			}
			dragBitmap->Unlock();
		} else {
			delete dragBitmap;
			dragBitmap = nullptr;
		}
		const BRect& frame = updateRect;
		if (dragBitmap != nullptr) {
			DragMessage(&message, dragBitmap, B_OP_ALPHA,
				BPoint(where.x - frame.left, where.y - frame.top));
		} else {
			DragMessage(&message, frame, this);
		}

		return true;
	}
	return false;
}


void
GTab::SetIsFront(bool isFront)
{
	if (fIsFront == isFront)
		return;

	fIsFront = isFront;
	Invalidate();
}


bool
GTab::IsFront() const
{
	return fIsFront;
}


void
GTab::OnDropMessage(BMessage* message)
{
	Container()->OnDropTab(this, message);
}


///////////////////////////////////////////////////////////////////////////////

//TODO MOve in TabUtils.
#define kCloseButtonWidth 19.0f
const int32 kBrightnessBreakValue = 126;

static void inline
IncreaseContrastBy(float& tint, const float& value, const int& brightness)
{
	tint *= 1 + ((brightness >= kBrightnessBreakValue) ? +1 : -1) * value;
}


// GTabCloseButton
GTabCloseButton::GTabCloseButton(const char* label,
										const BHandler* handler):
										GTab(label),
										fOverCloseRect(false),
										fClicked(false),
										fHandler(handler)
{
}


// TODO: we should better understand how to extend the default sizes.
BSize
GTabCloseButton::MinSize()
{
	return GTab::MinSize();
}


BSize
GTabCloseButton::MaxSize()
{
	BSize s(GTab::MaxSize());
	//s.width += kCloseButtonWidth;
	return s;
}


void
GTabCloseButton::DrawContents(BView* owner, BRect frame,
										const BRect& updateRect, bool isFront)
{
	BRect labelFrame = frame;
	labelFrame.right -= kCloseButtonWidth;
	DrawLabel(owner, labelFrame, updateRect, isFront);
	frame.left = labelFrame.right;
	//FillRect(frame);
	DrawCloseButton(owner, frame, updateRect, isFront);
}


void
GTabCloseButton::MouseDown(BPoint where)
{
	BMessage* msg = Window()->CurrentMessage();
	if (msg == nullptr)
		return;

	const int32 buttons = msg->GetInt32("buttons", 0);
	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		BRect closeRect = RectCloseButton();
		bool inside = closeRect.Contains(where);
		if (inside != fClicked) {
			fClicked = inside;
			Invalidate(closeRect);
			return;
		}
	} else if (buttons & B_TERTIARY_MOUSE_BUTTON) {
		CloseButtonClicked();
	}
	GTab::MouseDown(where);
}


void
GTabCloseButton::MouseUp(BPoint where)
{
	if (fClicked) {
		fClicked = false;
		Invalidate();
		BRect closeRect = RectCloseButton();
		bool inside = closeRect.Contains(where);
		if (inside && Container()) {
			CloseButtonClicked();
		}
	}
	GTab::MouseUp(where);
}


void
GTabCloseButton::MouseMoved(BPoint where, uint32 transit,
										const BMessage* dragMessage)
{
	GTab::MouseMoved(where, transit, dragMessage);
	if (dragMessage == nullptr) {
		BRect closeRect = RectCloseButton();
		bool inside = closeRect.Contains(where);
		if (inside != fOverCloseRect) {
			fOverCloseRect = inside;
			if (inside == false)
				fClicked = false;
			Invalidate(closeRect);
		}
	}
}


BRect
GTabCloseButton::RectCloseButton()
{
	BRect frame  = Bounds();
	frame.right -= be_control_look->DefaultLabelSpacing();
	frame.left = frame.right - kCloseButtonWidth + 3;
	frame.top += (frame.Height()/2.0f) - kCloseButtonWidth/2.0 + TabViewTools::DefaultFontDescent();
	frame.bottom = frame.top + frame.Width();
	return frame;
}


void
GTabCloseButton::CloseButtonClicked()
{
	// In async messages better to use index or id
	// to avoid concurrency problem
	// (in this case: a process is removing tabs while I'm pressing close)
	BMessage msg(kTVCloseTab);
	msg.AddInt32("index", Container()->IndexOfTab(this));
	BMessenger(fHandler).SendMessage(&msg);
}



void
GTabCloseButton::DrawCloseButton(BView* owner, BRect buttonRect, const BRect& updateRect,
							bool isFront)
{
	BRect closeRect = RectCloseButton();
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	float tint = B_LIGHTEN_1_TINT;
	if (base.Brightness() >= kBrightnessBreakValue) {
		tint = B_DARKEN_1_TINT *1.2;
	}

	if (fOverCloseRect) {
		// Draw the button frame
		be_control_look->DrawButtonFrame(owner, closeRect, updateRect,
			base, base,
			BControlLook::B_ACTIVATED | BControlLook::B_BLEND_FRAME);

		rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
		be_control_look->DrawButtonBackground(owner, closeRect, updateRect,
			background, BControlLook::B_ACTIVATED);
	} else {
		closeRect.top += 4;
		closeRect.left += 4;
		closeRect.right -= 2;
		closeRect.bottom -= 2;
	}

	// Draw the ×
	if (fClicked)
		IncreaseContrastBy(tint, .2, base.Brightness());

	base = tint_color(base, tint);
	owner->SetHighColor(base);
	owner->SetPenSize(2);

	owner->StrokeLine(closeRect.LeftTop(), closeRect.RightBottom());
	owner->StrokeLine(closeRect.LeftBottom(), closeRect.RightTop());
	owner->SetPenSize(1);
}


// Filler
Filler::Filler(TabsContainer* tabsContainer)
	: BView("_filler_", B_WILL_DRAW)
{
	SetContainer(tabsContainer);
}


void
Filler::Draw(BRect rect)
{
	BRect bounds(Bounds());
	TabViewTools::DrawTabBackground(this, bounds, rect);
	DropZoneDraw(this, bounds);
}


void
Filler::MouseUp(BPoint where)
{
	DropZoneMouseUp(this, where);
}


void
Filler::MessageReceived(BMessage* message)
{
	if (DropZoneMessageReceived(message) == false)
		BView::MessageReceived(message);
}


void
Filler::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	DropZoneMouseMoved(this, where, transit, dragMessage);
}


void
Filler::OnDropMessage(BMessage* message)
{
	Container()->OnDropTab(nullptr, message);
}
