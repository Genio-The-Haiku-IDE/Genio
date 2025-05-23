/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 *
 * Adapted from Debugger::ConsoleOutputView class
 * Copyright 2013-2014, Rene Gollent, rene@gollent.com
 *
 * Distributed under the terms of the MIT License
 */


#include "ConsoleIOView.h"

#include <AutoDeleter.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <String.h>

#include "ConfigManager.h"
#include "ConsoleIOThread.h"
#include "GenioApp.h"
#include "WordTextView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConsoleIOView"

enum {
	MSG_CLEAR_OUTPUT	= 'clou',
	MSG_POST_OUTPUT		= 'poou',
	MSG_STOP_PROCESS	= 'stpr',
	MSG_RUN_PROCESS		= 'runp'
};

struct ConsoleIOView::OutputInfo {
	int32 	fd;
	BString	text;

	OutputInfo(int32 fd, const BString& text)
		:
		fd(fd),
		text(text)
	{
	}
};


ConsoleIOView::ConsoleIOView(const BString& name)
	:
	BGroupView(B_VERTICAL, 0.0f)
	, fConsoleIOText(nullptr)
	, fPendingOutput(nullptr)
	, fConsoleIOThread(nullptr)
{
	SetName(name);

	try {
		_Init();
	} catch (...) {
		throw;
	}

	SetFlags(Flags() | B_PULSE_NEEDED);
}


ConsoleIOView::~ConsoleIOView()
{
	_StopThreads();
	if (fPendingOutput) {
		fPendingOutput->clear();
		delete fPendingOutput;
	}
}

status_t
ConsoleIOView::RunCommand(BMessage* cmd_message)
{
	cmd_message->what = MSG_RUN_PROCESS;
	return BMessenger(this).SendMessage(cmd_message);
}


void
ConsoleIOView::Pulse()
{
	if (fConsoleIOThread &&
		fConsoleIOThread->IsDone()) {
			_StopCommand(B_OK);
	}
}


void
ConsoleIOView::_StopThreads()
{
	if (fConsoleIOThread == NULL)
		return;

	fConsoleIOThread->InterruptExternal();
	delete fConsoleIOThread;
	fConsoleIOThread = nullptr;
}


void
ConsoleIOView::_StopCommand(status_t status)
{
	if (fConsoleIOThread != nullptr) {
		const BMessage* dataStore = fConsoleIOThread->GetDataStore();
		BString cmdType = dataStore->GetString("cmd_type");
		BString projectName = dataStore->GetString("project_name");
		_StopThreads();

		_BannerMessage("ended   --");

		fStopButton->SetEnabled(false);
		BMessage message(CONSOLEIOTHREAD_EXIT);
		message.AddString("cmd_type", cmdType);
		message.AddString("project_name", projectName);
		message.AddInt32("status", status);
		Window()->PostMessage(&message);
		fBannerClaim = "";
	}
}


void
ConsoleIOView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case CONSOLEIOTHREAD_STDERR: {
			BString string;
			if (message->FindString("stderr",  &string) == B_OK)
				ConsoleOutputReceived(2, string);
			break;
		}
		case CONSOLEIOTHREAD_STDOUT: {
			BString string;
			if (message->FindString("stdout",  &string) == B_OK)
				ConsoleOutputReceived(1, string);
			break;
		}
		case MSG_CLEAR_OUTPUT:
		{
			fConsoleIOText->SetText("");
			fPendingOutput->clear();

			// Used to reload settings too
			SetWordWrap(gCFG["wrap_console"]);
			fBannerEnabled->SetValue(gCFG["console_banner"]);
			break;
		}
		case MSG_POST_OUTPUT:
		{
			auto it = fPendingOutput->begin();
			if (it == fPendingOutput->end())
				break;

			_HandleConsoleOutput((*it).get());

			fPendingOutput->erase(it);

			break;
		}
		case MSG_RUN_PROCESS:
		{
			if (fConsoleIOThread != nullptr && !fConsoleIOThread->IsDone()) {
				// TODO: Horrible hack to be able to stop and relaunch build.
				// should be done differently
				BString runningCmdType = fConsoleIOThread->GetDataStore()->GetString("cmd_type");
				BString cmdType;
				if (message->FindString("cmd_type", &cmdType) == B_OK
						&& cmdType.Compare("build") == 0 && runningCmdType == "build") {
					_StopThreads();

					BString msg = "\n *** ";
					msg << B_TRANSLATE("Restarting build" B_UTF8_ELLIPSIS);
					msg << "\n";
					ConsoleOutputReceived(1, msg);
				} else {
					//this should be prevented by the UI...
					BString msg = "\n *** ";
					msg << B_TRANSLATE("Another command is running.");
					msg << "\n";

					ConsoleOutputReceived(1, msg);
					return;
				}
			}
			fStopButton->SetEnabled(true);
			fConsoleIOThread = new ConsoleIOThread(message, BMessenger(this));
			fConsoleIOThread->Start();
			fBannerClaim = message->GetString("banner_claim");
			_BannerMessage("started   ");

			break;
		}
		case CONSOLEIOTHREAD_EXIT:
		case MSG_STOP_PROCESS:
		{
			status_t status = message->GetInt32("status", B_OK);
			thread_id pid = message->GetInt32("pid", -1);
			if (pid > 0) {
				int result = 0;
				if (waitpid(pid, &result, WNOHANG) > 0) {
					status = WIFEXITED(result) ?
						( WEXITSTATUS(result) == 0 ? B_OK : B_ERROR) : B_ERROR;
				}
			}
			_StopCommand(status);
			break;
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ConsoleIOView::_BannerMessage(BString status)
{
	if (!gCFG["console_banner"])
		return;

	BString banner;
	banner  << "--------------------------------"
			<< "   "
			<< fBannerClaim
			<< " "
			<< status
			<< "--------------------------------\n";

	ConsoleOutputReceived(1, banner);
}


void
ConsoleIOView::AttachedToWindow()
{
	BGroupView::AttachedToWindow();

	fStdoutEnabled->SetValue(B_CONTROL_ON);
	fStderrEnabled->SetValue(B_CONTROL_ON);
	fWrapEnabled->SetEnabled(false);
	fBannerEnabled->SetEnabled(false);

	SetWordWrap(gCFG["wrap_console"]);

	fBannerEnabled->SetValue(gCFG["console_banner"]);

	fClearButton->SetTarget(this);
	fStopButton->SetTarget(this);
	fStopButton->SetEnabled(false);
}


void
ConsoleIOView::Clear()
{
	BMessenger(this).SendMessage(MSG_CLEAR_OUTPUT);
}


void
ConsoleIOView::ConsoleOutputReceived(int32 fd, const BString& output)
{
	if (fd == 1 && fStdoutEnabled->Value() != B_CONTROL_ON)
		return;
	else if (fd == 2 && fStderrEnabled->Value() != B_CONTROL_ON)
		return;

	fPendingOutput->push_back(std::make_unique<OutputInfo>(fd, output));

	BMessenger(this).SendMessage(MSG_POST_OUTPUT);
}


void
ConsoleIOView::EnableStopButton(bool doIt)
{
	fStopButton->SetEnabled(doIt);
}


void
ConsoleIOView::SetWordWrap(bool wrap)
{
	fConsoleIOText->SetWordWrap(wrap);
	fWrapEnabled->SetValue(wrap ? B_CONTROL_ON : B_CONTROL_OFF);
}


BTextView*
ConsoleIOView::TextView()
{
	return fConsoleIOText;
}


void
ConsoleIOView::_Init()
{
	fPendingOutput = new OutputInfoList();

	fConsoleIOText = new WordTextView("console_io");
	fConsoleIOText->SetStylable(true);
	fConsoleIOText->SetDoesUndo(false);
	fConsoleIOText->MakeEditable(false);

	BScrollView* consoleScrollView  = new BScrollView("console_scroll", fConsoleIOText, 0,
			true, true);

	fStdoutEnabled = new BCheckBox(B_TRANSLATE("stdout"));
	fStderrEnabled = new BCheckBox(B_TRANSLATE("stderr"));
	fWrapEnabled = new BCheckBox(B_TRANSLATE_COMMENT("Wrap", "As in wrapping long lines. Short as possible."));
	fBannerEnabled = new BCheckBox(B_TRANSLATE_COMMENT("Banner",
		"A separating line inserted at the start and end of a command output in the console. Short as possible."));
	fClearButton = new BButton(B_TRANSLATE("Clear"), new BMessage(MSG_CLEAR_OUTPUT));
	fStopButton = new BButton(B_TRANSLATE("Stop"), new BMessage(MSG_STOP_PROCESS));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
		.Add(consoleScrollView, 3.0f)
		.AddGroup(B_VERTICAL, 0.0f)
			.SetInsets(B_USE_SMALL_SPACING)
			.Add(fStdoutEnabled)
			.Add(fStderrEnabled)
			.Add(fWrapEnabled)
			.Add(fBannerEnabled)
			.AddGlue()
			.Add(fClearButton)
			.Add(fStopButton)
		.End()
	.End();

}


void
ConsoleIOView::_HandleConsoleOutput(OutputInfo* info)
{
	if (info->fd == 1 && fStdoutEnabled->Value() != B_CONTROL_ON)
		return;
	else if (info->fd == 2 && fStderrEnabled->Value() != B_CONTROL_ON)
		return;

	text_run_array run;
	run.count = 1;
	run.runs[0].font = be_fixed_font;
	run.runs[0].offset = 0;
	// stderr goes orange
	if (info->fd == 2) {
		run.runs[0].color = ui_color(B_FAILURE_COLOR);
	} else {
		run.runs[0].color = ui_color(B_LIST_ITEM_TEXT_COLOR);
	}
	run.runs[0].color.alpha = 255;

	bool autoScroll = false;
	BScrollBar* scroller = fConsoleIOText->ScrollBar(B_VERTICAL);
	float min, max;
	scroller->GetRange(&min, &max);
	if (min == max || scroller->Value() == max)
		autoScroll = true;

	fConsoleIOText->Insert(fConsoleIOText->TextLength(), info->text,
		info->text.Length(), &run);
	if (autoScroll) {
		scroller->GetRange(&min, &max);
		fConsoleIOText->ScrollTo(0.0, max);
	}
}
