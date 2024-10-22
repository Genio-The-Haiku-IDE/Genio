/*
 * Copyright 2013-2024, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GlobalStatusView.h"

#include <BarberPole.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <MessageRunner.h>
#include <StringView.h>
#include <Window.h>

#include "GenioWindowMessages.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GlobalStatusView"


const bigtime_t kTextAutohideTimeout = 5000000ULL;

const uint32 kHideText = 'HIDE';

GlobalStatusView::GlobalStatusView()
	:
	BView("global_status_view", B_WILL_DRAW),
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

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.AddGlue()
		.Add(fStringView)
		.Add(fBarberPole);
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
		case kHideText:
			fStringView->SetText("");
			break;
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 what;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
			switch (what) {
				case MSG_NOTIFY_BUILDING_PHASE:
				{
					// TODO: Instead of doing this here, put the string into the message
					// from the caller and just retrieve it and display it here
					bool building = message->GetBool("building", false);
					BString projectName = message->GetString("project_name");
					BString cmdType = message->GetString("cmd_type");
					BString text;
					if (building) {
						if (cmdType.Compare("build") == 0)
							text = B_TRANSLATE("Building '\"%project%\"'" B_UTF8_ELLIPSIS);
						else if (cmdType.Compare("clean"))
							text = B_TRANSLATE("Cleaning '\"%project%\"'" B_UTF8_ELLIPSIS);
						fDontHideText = true;
						fBarberPole->Start();
					} else {
						if (cmdType.Compare("build") == 0)
							text = B_TRANSLATE("Finished building '\"%project%\"'");
						else
							text = B_TRANSLATE("Finished cleaning '\"%project%\"'");
						fDontHideText = false;
						fBarberPole->Stop();
						BMessenger messenger(this);
						BMessageRunner::StartSending(messenger, new BMessage(kHideText),
									kTextAutohideTimeout, 1);
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
