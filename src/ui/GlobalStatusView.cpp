/*
 * Copyright 2013-2024, Stefano Ceccherini 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GlobalStatusView.h"

#include <BarberPole.h>
#include <Box.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <StatusBar.h>
#include <StringView.h>
#include <Window.h>

#include "GenioWindowMessages.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GlobalStatusView"


const bigtime_t kTextAutohideTimeout = 5000000ULL;


GlobalStatusView::GlobalStatusView()
	:
	BView("global_status_view", B_WILL_DRAW|B_PULSE_NEEDED),
	fBarberPole(nullptr),
	fStringView(nullptr),
	fLastStatusChange(system_time()),
	fDontHideText(false)
{
	fBarberPole = new BarberPole("barber pole");
	//fBarberPole->SetExplicitMinSize(BSize(100, B_SIZE_UNLIMITED));
	fBarberPole->SetExplicitMaxSize(BSize(250, B_SIZE_UNLIMITED));
	fBarberPole->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));
	fStringView = new BStringView("text", "");
	fStringView->SetExplicitMinSize(BSize(200, B_SIZE_UNSET));
	fStringView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));

	BBox* box = new BBox("box");
	box->SetBorder(B_FANCY_BORDER);
	BLayoutBuilder::Group<>(box, B_HORIZONTAL)
		.SetInsets(8, 0, 0, 0)
		.Add(fStringView)
		.Add(fBarberPole);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.AddGlue()
		.Add(box)
		.End();
}


void
GlobalStatusView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_BUILDING_PHASE);
		Window()->UnlockLooper();
	}
}


/* virtual */
void
GlobalStatusView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
	if (Window()->LockLooper()) {
		Window()->StopWatching(this, MSG_NOTIFY_BUILDING_PHASE);
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
					BString projectName = message->GetString("project_name");
					BString text;
					if (building) {
						text = B_TRANSLATE("Building \"%project%\"" B_UTF8_ELLIPSIS);
						fDontHideText = true;
						fBarberPole->Start();
					} else {
						text = B_TRANSLATE("Finished building \"%project%\"");
						fDontHideText = false;
						fBarberPole->Stop();
					}
					text.ReplaceFirst("\"%project%\"", projectName);
					fStringView->SetText(text.String());

					fLastStatusChange = system_time();
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
	if (!fDontHideText && system_time() >= fLastStatusChange + kTextAutohideTimeout) {
		fStringView->SetText("");
	}
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
