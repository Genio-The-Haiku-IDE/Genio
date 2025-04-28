/*
 * Copyright 2013-2024, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GlobalStatusView.h"

#include <BarberPole.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <MessageRunner.h>
#include <StatusBar.h>
#include <StringView.h>
#include <Window.h>

#include "GenioWindowMessages.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GlobalStatusView"


const bigtime_t kTextAutohideTimeout = 5000000ULL;

const uint32 kHideBuildingText = 'HIDE';
const uint32 kHideFindText = 'HIFE';

GlobalStatusView::GlobalStatusView()
	:
	BView("global_status_view", B_WILL_DRAW),
	fBuildBarberPole(nullptr),
	fBuildStringView(nullptr),
	fLSPStringView(nullptr),
	fLSPStatusBar(nullptr),
	fLastStatusChange(system_time()),
	fRunnerBuild(nullptr),
	fRunnerFind(nullptr)
{
	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float height = ::ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading + be_control_look->DefaultItemSpacing());

	fBuildBarberPole = new BarberPole("build_barberpole");
	fBuildStringView = new BStringView("build_text", "");
	fLSPStringView = new BStringView("LSP_text", "");
	fLSPStatusBar = new BStatusBar("LSP_progressbar");
	fLastFindStatus = new BStringView("find_status", "");

	fBuildBarberPole->Hide();

	fLSPStatusBar->Hide();

	// TODO: Maybe this is wrong but it works
	SetExplicitMaxSize(BSize(B_SIZE_UNSET, height));
	SetExplicitMinSize(BSize(B_SIZE_UNSET, height));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 0.4)
			.SetInsets(2, 0)
			.Add(fLSPStringView)
			.Add(fLSPStatusBar)
		.End()
		.AddGlue(0.1)
		.Add(fLastFindStatus, 0.2)
		.AddGlue(0.1)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 0.4)
			.SetInsets(2, 4)
			.Add(fBuildStringView)
			.Add(fBuildBarberPole)
		.End()
		;

	fBuildStringView->SetExplicitMinSize(BSize(200, B_SIZE_UNSET));
	fBuildStringView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET));

	fBuildBarberPole->SetExplicitMaxSize(BSize(250, B_SIZE_UNSET));
	fBuildBarberPole->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));

	fLSPStringView->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));
	fLSPStringView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET));

	fLSPStatusBar->SetExplicitMaxSize(BSize(150, B_SIZE_UNSET));
	fLSPStatusBar->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
}


void
GlobalStatusView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_BUILDING_PHASE);
		Window()->StartWatching(this, MSG_NOTIFY_LSP_INDEXING);
		Window()->StartWatching(this, MSG_NOTIFY_FIND_STATUS);
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
		Window()->StopWatching(this, MSG_NOTIFY_LSP_INDEXING);
		Window()->StopWatching(this, MSG_NOTIFY_FIND_STATUS);
		Window()->UnlockLooper();
	}
}


void
GlobalStatusView::Draw(BRect updateRect)
{
	rgb_color baseColor = LowColor();
	BRect bounds = Bounds();
	be_control_look->DrawBorder(this, bounds, updateRect, baseColor, B_FANCY_BORDER,
		0, BControlLook::B_TOP_BORDER);
	BView::Draw(updateRect);
}


void
GlobalStatusView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kHideBuildingText:
			_ResetRunner(&fRunnerBuild);
			fBuildStringView->SetText("");
			fBuildBarberPole->Hide();
			break;
		case kHideFindText:
			_ResetRunner(&fRunnerFind);
			fLastFindStatus->SetText("");
			break;
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 what;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
			switch (what) {
				case MSG_NOTIFY_BUILDING_PHASE:
				{
					_ResetRunner(&fRunnerBuild);

					if (fBuildBarberPole->IsHidden())
						fBuildBarberPole->Show();

					// TODO: Instead of doing this here, put the string into the message
					// from the caller and just retrieve it and display it here
					bool building = message->GetBool("building", false);
					BString projectName = message->GetString("project_name");
					BString cmdType = message->GetString("cmd_type");
					status_t status = message->GetInt32("status", B_OK);
					BString text;
					if (building) {
						if (cmdType.Compare("build") == 0)
							text = B_TRANSLATE("Building project '\"%project%\"'" B_UTF8_ELLIPSIS);
						else if (cmdType.Compare("clean") == 0)
							text = B_TRANSLATE("Cleaning project '\"%project%\"'" B_UTF8_ELLIPSIS);
						fBuildBarberPole->Start();
					} else {
						if (cmdType.Compare("build") == 0) {
							if (status == B_OK)
								text = B_TRANSLATE("Finished building project '\"%project%\"'");
							else
								text = B_TRANSLATE("Failed building project '\"%project%\"'");
						} else if (cmdType.Compare("clean") == 0) {
							if (status == B_OK)
								text = B_TRANSLATE("Finished cleaning project '\"%project%\"'");
							else
								text = B_TRANSLATE("Failed cleaning project '\"%project%\"'");
						}
						fBuildBarberPole->Stop();
						_StartRunner(&fRunnerBuild, kHideBuildingText);
					}
					text.ReplaceFirst("\"%project%\"", projectName);
					fBuildStringView->SetText(text.String());

					if (status != B_OK) {
						// On fail
						fBuildStringView->SetHighColor(ui_color(B_FAILURE_COLOR));
						// beep();
					} else
						fBuildStringView->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));

					fLastStatusChange = system_time();
					break;
				}
				case MSG_NOTIFY_LSP_INDEXING:
				{
					BString kind = message->GetString("kind", "end");
					if (kind.Compare("end") == 0) {
						fLSPStringView->SetText("");
						if (!fLSPStatusBar->IsHidden())
							fLSPStatusBar->Hide();
						return;
					}

					// TODO: translate ?
					BString text;
					const char* str = nullptr;
					if (message->FindString("title", &str) == B_OK) {
						text << str << " ";
					}
					if (message->FindString("message", &str) == B_OK) {
						text << str << " ";
					}
					int32 percentage = 0;
					if (message->FindInt32("percentage", &percentage) == B_OK) {
						text << "(" << percentage << "%)";
						if (fLSPStatusBar->IsHidden())
							fLSPStatusBar->Show();

						fLSPStatusBar->Update(percentage - fLSPStatusBar->CurrentValue());
					}

					fLSPStringView->SetText(text.String());
					break;
				}
				case MSG_NOTIFY_FIND_STATUS:
				{
					_ResetRunner(&fRunnerFind);
					fLastFindStatus->SetText(message->GetString("status", ""));
					_StartRunner(&fRunnerFind, kHideFindText);
					break;
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
GlobalStatusView::_ResetRunner(BMessageRunner** runner)
{
	if (*runner != nullptr) {
		delete *runner;
		*runner = nullptr;
	}
}


void
GlobalStatusView::_StartRunner(BMessageRunner** runner, uint32 what)
{
	BMessenger messenger(this);
	*runner = new BMessageRunner(messenger, new BMessage(what),
				kTextAutohideTimeout, 1);
}
