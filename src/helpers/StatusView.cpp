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
 * Distributed under the terms of the MIT License.
 */


#include "StatusView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Catalog.h>
#include <ControlLook.h>
//#include <DirMenu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Window.h>


const float kHorzSpacing = 5.f;

using namespace BPrivate;


namespace controls {

StatusView::StatusView(BScrollView* scrollView)
			:
			BView(BRect(), "statusview",
				B_FOLLOW_BOTTOM | B_FOLLOW_LEFT, B_WILL_DRAW),
			fScrollView(scrollView),
			fPreferredSize(0., 0.)
{
	scrollView->AddChild(this);
}


StatusView::~StatusView()
{
}


void
StatusView::AttachedToWindow()
{
	BScrollBar* scrollBar = fScrollView->ScrollBar(B_HORIZONTAL);
	MoveTo(0., scrollBar->Frame().top);

	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	ResizeToPreferred();
}


void
StatusView::GetPreferredSize(float* _width, float* _height)
{
	_ValidatePreferredSize();

	if (_width)
		*_width = fPreferredSize.width;

	if (_height)
		*_height = fPreferredSize.height;
}


void
StatusView::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);

	if (Bounds().Width() > width)
		width = Bounds().Width();

	BView::ResizeTo(width, height);
}


void
StatusView::WindowActivated(bool active)
{
	// Workaround: doesn't redraw automatically
	Invalidate();
}


BScrollView*
StatusView::ScrollView()
{
	return fScrollView;
}


void
StatusView::_ValidatePreferredSize()
{
	// width
	fPreferredSize.width = Width();

	// height
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	fPreferredSize.height = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);

	if (fPreferredSize.height < B_H_SCROLL_BAR_HEIGHT + 1)
		fPreferredSize.height = B_H_SCROLL_BAR_HEIGHT + 1;

	ResizeBy(fPreferredSize.width, 0);
	BScrollBar* scrollBar = fScrollView->ScrollBar(B_HORIZONTAL);
	float diff = scrollBar->Frame().left - fPreferredSize.width;
	if(fabs(diff) > 0.5) {
		scrollBar->ResizeBy(diff, 0);
		scrollBar->MoveBy(-diff, 0);
	}
}

} // namespace controls
