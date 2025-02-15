/*
 * Copyright 2023-2024, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MTermView.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <String.h>
#include <MessageRunner.h>

#include "ConfigManager.h"
#include "KeyTextViewScintilla.h"
#include "Log.h"
#include "MTerm.h"
#include "Styler.h"

extern ConfigManager gCFG;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TermView"

enum {
	kTermViewRun	= 'tvru',
	kTermViewClear	= 'tvcl',
	kTermViewStop	= 'tvst',

	kTermViewWrap	= 'tvwr',
	kTermViewBanner	= 'tvba',

	kTermFlushBuffer = 'flux'
};

static const bigtime_t  kFlushInterval = 1000000; //1sec
static const int32 		kFlushSize = 1024; //1sec



MTermView::MTermView(const BString& name, const BMessenger& target)
	:
	BGroupView(B_VERTICAL, 0.0f)
	, fWindowTarget(target)
	, fKeyTextView(nullptr)
	, fMTerm(nullptr)
	, fWrapEnabled(nullptr)
	, fBannerEnabled(nullptr)
	, fBufferFlusher(nullptr)
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
MTermView::ApplyStyle()
{
	BFont font = be_fixed_font;
	const BString fontFamily = gCFG["edit_fontfamily"];
	if (!fontFamily.IsEmpty()){
		font.SetFamilyAndStyle(fontFamily, nullptr);
	}
	int32 fontSize = gCFG["edit_fontsize"];
	if (fontSize > 0)
		font.SetSize(fontSize);
	BString style = gCFG["console_style"];
	if (style.Compare(B_TRANSLATE("(follow system style)")) == 0) {
		Styler::ApplySystemStyle(fKeyTextView);
	} else {
		if (style.Compare(B_TRANSLATE("(follow editor style)")) == 0)
			style = BString(gCFG["editor_style"]);

		Styler::ApplyBasicStyle(fKeyTextView, style, &font);
	}
}


void
MTermView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code) != B_OK)
				break;
			if (code == gCFG.UpdateMessageWhat()) {
				BString key = message->GetString("key", "");
				if (key.Compare("console_style") == 0) {
					ApplyStyle();
				}
			}
			break;
		}
		case kTermViewClear:
		{
			TextView()->ClearAll();
			TextView()->ClearBuffer();
			break;
		}
		case kMTOutputText:
		{
			BString info = message->GetString("text","");
			_HandleOutput(info);
			break;
		}
		case kKTVInputBuffer:
		{
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
			argv[0] = ::strdup("/bin/sh");
			argv[1] = ::strdup("-c");
			argv[2] = ::strdup(cmd.String());
			argv[argc] = nullptr;

			fMTerm->Run(1, argv);
			delete[] argv;
			_BannerMessage("started   ");
			break;
		}
		case Genio::Task::TASK_RESULT_MESSAGE:
		{
			// The task is ending. Let's avoid calling _EnsureStopped() (which terminates the task)
			// before it has fully completed, to prevent potentially entering an inconsistent
			// kernel lock state.

			thread_id taskId = (thread_id)message->GetInt32("TaskResult::TaskID", -1);
			if (taskId > -1) {
				status_t threadStatus = B_OK;
				status_t waitStatus = wait_for_thread(taskId, &threadStatus);
				LogInfo("TASK_RESULT_MESSAGE for thread %d: threadStatus (%s) waitStatus (%s)",
						taskId,
						strerror(threadStatus),
						strerror(waitStatus));
				_EnsureStopped();
			}


			break;
		}
		case kTermViewStop:
		{
			_EnsureStopped();
			break;
		}
		case kTermViewWrap:
		{
			if (fWrapEnabled->Value() == B_CONTROL_ON) {
				fKeyTextView->SendMessage(SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
			} else {
				fKeyTextView->SendMessage(SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
			}
			break;
		}
		case kTermViewBanner:
		{
			break;
		}
		case kTermFlushBuffer:
			_FlushOutput();
		break;
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
MTermView::_EnsureStopped()
{
	if (fMTerm != nullptr) {
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
	if (fBannerEnabled->Value() == B_CONTROL_OFF)
		return;

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

	fBannerEnabled->SetTarget(this);
	fWrapEnabled->SetTarget(this);

	ApplyStyle();
	be_app->StartWatching(this, gCFG.UpdateMessageWhat());
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
	fKeyTextView = new KeyTextViewScintilla("KeyTextViewScintilla", BMessenger(this));

	fWrapEnabled = new BCheckBox(B_TRANSLATE_COMMENT("Wrap", "As in wrapping long lines. Short as possible."),
					new BMessage(kTermViewWrap));
	fBannerEnabled = new BCheckBox(B_TRANSLATE_COMMENT("Banner",
		"A separating line inserted at the start and end of a command output in the console. Short as possible."),
					new BMessage(kTermViewBanner));

	fClearButton = new BButton(B_TRANSLATE("Clear"), new BMessage(kTermViewClear));
	fStopButton = new BButton(B_TRANSLATE("Stop"), new BMessage(kTermViewStop));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
		.Add(fKeyTextView, 3.0f)
		.AddGroup(B_VERTICAL, 0.0f)
			.SetInsets(B_USE_SMALL_SPACING)
			.Add(fWrapEnabled)
			.Add(fBannerEnabled)
			.AddGlue()
			.Add(fClearButton)
			.Add(fStopButton)
		.End()
	.End();

	EnableStopButton(false);

	fBannerEnabled->SetValue(B_CONTROL_ON);
	fWrapEnabled->SetValue(B_CONTROL_ON);
	fKeyTextView->SendMessage(SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
}


void
MTermView::_HandleOutput(const BString& info)
{
	fOutputBuffer.Append(info);
	if (fOutputBuffer.Length() > kFlushSize) {
		_FlushOutput();
	} else {
		static BMessage flush(kTermFlushBuffer);
		if (fBufferFlusher == nullptr || fBufferFlusher->SetInterval(kFlushInterval) != B_OK) {
			if (fBufferFlusher != nullptr)
				delete fBufferFlusher;
			fBufferFlusher = new BMessageRunner(BMessenger(this), &flush, kFlushInterval, 1);
		}
	}
}

void
MTermView::_FlushOutput()
{
	fKeyTextView->Append(fOutputBuffer);
	fOutputBuffer = "";
}

KeyTextViewScintilla*
MTermView::TextView()
{
	return fKeyTextView;
}
