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
#include <StringView.h>
#include <Window.h>

#include "GenioWindowMessages.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GlobalStatusView"



GlobalStatusView::GlobalStatusView()
	:
	BView("global_status_view", B_WILL_DRAW|B_PULSE_NEEDED),
	fStatusBar(nullptr),
	fStringView(nullptr)
{
	fStatusBar = new BStatusBar("progress_bar", "");
	fStatusBar->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));

	fStringView = new BStringView("text", "");
	fStringView->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(0)
		.Add(fStringView)
		.Add(fStatusBar);
	// TODO:
	fStatusBar->Hide();
}


void
GlobalStatusView::AttachedToWindow()
{
	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_BUILDING_PHASE);
		Window()->UnlockLooper();
	}
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
				case MSG_NOTIFY_BUILDING_PHASE:
				{
					bool building = message->GetBool("building", false);
					if (building)
						fStringView->SetText(B_TRANSLATE("Building" B_UTF8_ELLIPSIS));
					else
						fStringView->SetText(B_TRANSLATE("Finished building"));
				}
				default:
					BView::MessageReceived(message);
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


