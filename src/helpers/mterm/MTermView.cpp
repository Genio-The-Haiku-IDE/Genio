/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MTermView.h"
#include "MTerm.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <String.h>
#include "KeyTextViewScintilla.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TermView"

enum {
	kTermViewRun	= 'tvru',
	kTermViewClear	= 'tvcl',
	kTermViewStop	= 'tvst',
};


MTermView::MTermView(const BString& name, const BMessenger& target)
	:
	BGroupView(B_VERTICAL, 0.0f)
	, fWindowTarget(target)
	, fKeyTextView(nullptr)
	, fMTerm(nullptr)
{
	SetName(name);
	_Init();
}


MTermView::~MTermView()
{
	_EnsureStopped();
}


status_t
MTermView::RunCommand(BMessage* cmd_message)
{
	cmd_message->what = kTermViewRun;
	return BMessenger(this).SendMessage(cmd_message);
}


void
MTermView::MessageReceived(BMessage* message)
{
	switch (message->what) {

		case kTermViewClear: {
			TextView()->ClearAll();
			TextView()->ClearBuffer();
			break;
		}
		case kMTOutputText: {
			BString info = message->GetString("text","");
			_HandleOutput(info);
			break;
		}
		case kKTVInputBuffer: {
			BString data = message->GetString("buffer", "");
			if (fMTerm != nullptr && data.Length() > 0)
				fMTerm->Write(data.String(), data.Length());
			break;
		}
		case kTermViewRun:
		{
			BString cmd = message->GetString("cmd", "");
			if (cmd.IsEmpty())
				break;
			_EnsureStopped();

			fBannerClaim = message->GetString("banner_claim", "");
			TextView()->ClearAll();
			TextView()->ClearBuffer();
			fKeyTextView->EnableInput(true);
			EnableStopButton(true);
			fMTerm = new MTerm(BMessenger(this));

			int32 argc = 3;
			const char** argv = new const char * [argc + 1];
			argv[0] = strdup("/bin/sh");
			argv[1] = strdup("-c");
			argv[2] = strdup(cmd.String());
			argv[argc] = nullptr;

			fMTerm->Run(1, argv);
			delete[] argv;
			_BannerMessage("started   ");

			break;
		}
		case Genio::Task::TASK_RESULT_MESSAGE:
		case kTermViewStop:
		{
			_EnsureStopped();
			break;
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}

void
MTermView::_EnsureStopped()
{
	if (fMTerm) {
		fMTerm->Kill();
		delete fMTerm;
		fMTerm = nullptr;
		fKeyTextView->EnableInput(false);
		EnableStopButton(false);
		_BannerMessage("ended   ");
		BMessage done(kTermViewDone);
		fWindowTarget.SendMessage(&done);
	}
	fBannerClaim = "";
}


void
MTermView::_BannerMessage(BString status)
{
	BString banner;
	banner  << "--------------------------------"
			<< "   "
			<< fBannerClaim
			<< " "
			<< status
			<< "--------------------------------\n";

	_HandleOutput(banner);
}


void
MTermView::AttachedToWindow()
{
	BGroupView::AttachedToWindow();

	fClearButton->SetTarget(this);
	fStopButton->SetTarget(this);
	fStopButton->SetEnabled(false);
}


void
MTermView::Clear()
{
	BMessenger(this).SendMessage(kTermViewClear);
}


void
MTermView::EnableStopButton(bool doIt)
{
	fStopButton->SetEnabled(doIt);
}


void
MTermView::_Init()
{
	fKeyTextView = new KeyTextViewScintilla("console_io", BMessenger(this));
	fClearButton = new BButton(B_TRANSLATE("Clear"), new BMessage(kTermViewClear));
	fStopButton = new BButton(B_TRANSLATE("Stop"), new BMessage(kTermViewStop));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
		.Add(fKeyTextView, 3.0f)
		.AddGroup(B_VERTICAL, 0.0f)
			.SetInsets(B_USE_SMALL_SPACING)
			.AddGlue()
			.Add(fClearButton)
			.Add(fStopButton)
		.End()
	.End();

	EnableStopButton(false);
}


void
MTermView::_HandleOutput(const BString& info)
{
	fKeyTextView->Append(info);
}


KeyTextViewScintilla*
MTermView::TextView()
{
	return fKeyTextView;
}
