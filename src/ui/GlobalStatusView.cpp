/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GlobalStatusView.h"


#include <Application.h>
#include <Bitmap.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <StatusBar.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GlobalStatusView"



GlobalStatusView::GlobalStatusView()
	:
	BView("global_status_view", B_WILL_DRAW|B_PULSE_NEEDED),
	fStatusBar(NULL)
{
	fStatusBar = new BStatusBar("progress_bar", "");
	fStatusBar->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));

	BView* statusView = BLayoutBuilder::Group<>()
		.SetInsets(0)
		.Add(fStatusBar)
		.View();

	BLayoutBuilder::Cards<>(this)
		.Add(statusView)
		.SetVisibleItem(int32(0));
}


void
GlobalStatusView::AttachedToWindow()
{
	if (be_app->LockLooper()) {

		be_app->UnlockLooper();
	}

	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
GlobalStatusView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
}


void
GlobalStatusView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 what;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
			switch (what) {
				default:
					break;
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
GlobalStatusView::Pulse()
{
	BView::Pulse();
}


BSize
GlobalStatusView::MinSize()
{
	return BView::MinSize();
}


BSize
GlobalStatusView::MaxSize()
{
	return BView::MaxSize();
}


