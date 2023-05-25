/*
 * Copyright (C) 2010 Rene Gollent <rene@gollent.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * Modified by:
 *		A. Mosca, amoscaster@gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TabView.h"

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <ControlLook.h>
#include <GroupView.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>
#include <Window.h>

#include "TabContainerView.h"


// #pragma mark - TabView



TabView::TabView()
	:
	fContainerView(NULL),
	fLayoutItem(new TabLayoutItem(this)),
	fLabel(),
	fIsFirst(false),
	fIsLast(false),
	fIsFront(false)
{
}


TabView::~TabView()
{
	// The layout item is deleted for us by the layout which contains it.
	if (!fContainerView)
		delete fLayoutItem;
}


BSize
TabView::MinSize()
{
	BSize size(MaxSize());
#if 0
	size.width = 60.0f;
#endif
	size.width = 200.0f;
	return size;
}


BSize
TabView::PreferredSize()
{
	return MaxSize();
}


BSize
TabView::MaxSize()
{
	float extra = be_control_look->DefaultLabelSpacing();
	float labelWidth = 300.0f;
	return BSize(labelWidth, _LabelHeight() + extra);
}


void
TabView::Draw(BRect updateRect)
{
	BRect frame(fLayoutItem->Frame());
	if (fIsFront) {
		// Extend the front tab outward left/right in order to merge
		// the frames of adjacent tabs.
		if (!fIsFirst)
			frame.left--;
		if (!fIsLast)
			frame.right++;
	}
	frame.bottom++;

	DrawBackground(fContainerView, frame, updateRect, fIsFirst, fIsLast,
		fIsFront);

	if (fIsFront) {
		frame.top += 0.0f;
		if (!fIsFirst)
			frame.left++;
		if (!fIsLast)
			frame.right--;
	} else
		frame.top += 3.0f;

	float spacing = be_control_look->DefaultLabelSpacing();
	frame.InsetBy(spacing, spacing / 2);
	DrawContents(fContainerView, frame, updateRect, fIsFirst, fIsLast,
		fIsFront);
}


void
TabView::DrawBackground(BView* owner, BRect frame, const BRect& updateRect,
	bool isFirst, bool isLast, bool isFront)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 borders = BControlLook::B_TOP_BORDER
		| BControlLook::B_BOTTOM_BORDER;

	if (isFirst)
		borders |= BControlLook::B_LEFT_BORDER;
	if (isLast)
		borders |= BControlLook::B_RIGHT_BORDER;
	if (isFront) {
		be_control_look->DrawActiveTab(owner, frame, updateRect, base,
			0, borders);
	} else {
		be_control_look->DrawInactiveTab(owner, frame, updateRect, base,
			0, borders);
	}
}


void
TabView::DrawContents(BView* owner, BRect frame, const BRect& updateRect,
	bool isFirst, bool isLast, bool isFront)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	be_control_look->DrawLabel(owner, fLabel.String(), frame, updateRect,
		base, 0, BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
}


void
TabView::MouseDown(BPoint where, uint32 buttons)
{
	fContainerView->SelectTab(this);
}


void
TabView::MouseUp(BPoint where)
{
}


void
TabView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
}


void
TabView::SetIsFront(bool isFront)
{
	Update(fIsFirst, fIsLast, isFront);
}


bool
TabView::IsFront() const
{
	return fIsFront;
}


void
TabView::SetIsLast(bool isLast)
{
	Update(fIsFirst, isLast, fIsFront);
}


void
TabView::Update(bool isFirst, bool isLast, bool isFront)
{
#if 1
	if (fIsFirst == isFirst && fIsLast == isLast && fIsFront == isFront)
		return;
#endif
	fIsFirst = isFirst;
	fIsLast = isLast;
	fIsFront = isFront;

	fLayoutItem->InvalidateContainer();
}


void
TabView::SetContainerView(TabContainerView* containerView)
{
	fContainerView = containerView;
}


TabContainerView*
TabView::ContainerView() const
{
	return fContainerView;
}


BLayoutItem*
TabView::LayoutItem() const
{
	return fLayoutItem;
}


void
TabView::SetLabel(const char* label)
{
	if (fLabel == label)
		return;
	fLabel = label;
	fLayoutItem->InvalidateLayout();
}


const BString&
TabView::Label() const
{
	return fLabel;
}


BRect
TabView::Frame() const
{
	return fLayoutItem->Frame();
}


float
TabView::_LabelHeight() const
{
	font_height fontHeight;
	fContainerView->GetFontHeight(&fontHeight);
	return ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);
}


// #pragma mark - TabLayoutItem


TabLayoutItem::TabLayoutItem(TabView* parent)
	:
	fParent(parent),
	fVisible(true)
{
}


bool
TabLayoutItem::IsVisible()
{
	return fVisible;
}


void
TabLayoutItem::SetVisible(bool visible)
{
	if (fVisible == visible)
		return;

	fVisible = visible;

	InvalidateContainer();
	fParent->ContainerView()->InvalidateLayout();
}


BRect
TabLayoutItem::Frame()
{
	return fFrame;
}


void
TabLayoutItem::SetFrame(BRect frame)
{
	BRect dirty = fFrame;
	fFrame = frame;
	dirty = dirty | fFrame;
	InvalidateContainer(dirty);
}


BView*
TabLayoutItem::View()
{
	return NULL;
}


BSize
TabLayoutItem::BaseMinSize()
{
	return fParent->MinSize();
}


BSize
TabLayoutItem::BaseMaxSize()
{
	return fParent->MaxSize();
}


BSize
TabLayoutItem::BasePreferredSize()
{
	return fParent->PreferredSize();
}


BAlignment
TabLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


TabView*
TabLayoutItem::Parent() const
{
	return fParent;
}


void
TabLayoutItem::InvalidateContainer()
{
	InvalidateContainer(Frame());
}


void
TabLayoutItem::InvalidateContainer(BRect frame)
{
	// Invalidate more than necessary, to help the TabContainerView
	// redraw the parts outside any tabs...
	frame.bottom++;
	frame.right++;
	fParent->ContainerView()->Invalidate(frame);
}
