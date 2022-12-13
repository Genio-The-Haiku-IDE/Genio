/*
 * Taken from Heidi project
 * Copyright 2014 Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "ShellView.h"

#include <Looper.h>

#include <stdio.h>

// #pragma mark - ExecThread

#define BUF_SIZE 512

class ExecThread : public BLooper {
public:
					ExecThread(BString command, BString dir, const BMessenger& target);
		void		MessageReceived(BMessage* msg);

private:
		BMessenger	fMessenger;
		BString		fCommand;
};


ExecThread::ExecThread(BString command, BString dir, const BMessenger& target)
	: BLooper(command.String())
{
	fCommand.SetToFormat("cd \"%s\" && %s 2>&1;", dir.String(), command.String());
	fMessenger = target;
}


void
ExecThread::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case ShellView::SV_LAUNCH:
	{
		FILE* fd = popen(fCommand.String(), "r");
		if (fd != NULL) {
			char buffer[BUF_SIZE];
			BMessage* newMsg = new BMessage(ShellView::SV_DATA);
			while (fgets(buffer, BUF_SIZE, fd)) {
				newMsg->RemoveName("data");
				newMsg->AddString("data", BString(buffer, BUF_SIZE).String());
				fMessenger.SendMessage(newMsg);
			}
			pclose(fd);
		}
		// TODO: error handling
		fMessenger.SendMessage(ShellView::SV_DONE);
		break;
	}
	default:
		BLooper::MessageReceived(msg);
	break;
	}
}

// #pragma mark - ShellView

ShellView::ShellView(const char* name, const BMessenger& messenger, uint32 what)
	: BTextView(name, be_fixed_font, NULL, B_WILL_DRAW | B_PULSE_NEEDED),
	fExecThread(nullptr),
//	  fExecDir("."),
//	  fCommand(""),
	fMessenger(messenger),
	fNotifyFinishedWhat(what)
{
	fScrollView = new BScrollView(name, this, B_NAVIGABLE, true, true);
	MakeEditable(false);
	SetWordWrap(false);
}


void
ShellView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case SV_DATA: {
			// Check if the vertical scroll bar is at the end
			float max;
			fScrollView->ScrollBar(B_VERTICAL)->GetRange(NULL, &max);
			bool atEnd = fScrollView->ScrollBar(B_VERTICAL)->Value() == max;

			BString str;
			msg->FindString("data", &str);
			BTextView::Insert(str.String());

			if (atEnd) {
				fScrollView->ScrollBar(B_VERTICAL)->GetRange(NULL, &max);
				fScrollView->ScrollBar(B_VERTICAL)->SetValue(max);
			}
		} break;
		case SV_DONE:
		{
			if (fExecThread->LockLooper()) {
				fExecThread->Quit();
				fExecThread = NULL;
			}
			BMessage doneMessage(fNotifyFinishedWhat);
			doneMessage.AddString("command", fCommand);
			fMessenger.SendMessage(&doneMessage);
			break;
		}
		default:
			BTextView::MessageReceived(msg);
		break;
	}
}


status_t
ShellView::SetCommand(BString command)
{
	if (fExecThread != NULL)
		return B_ERROR;

	fCommand = command;
	return B_OK;
}


status_t
ShellView::SetExecDir(BString execDir)
{
	if (fExecThread != NULL)
		return B_ERROR;

	fExecDir = execDir;
	return B_OK;
}


status_t
ShellView::Exec()
{
	if (fExecThread != NULL)
		return B_ERROR;

	fExecThread = new ExecThread(fCommand, fExecDir, BMessenger(this));
	fExecThread->Run();
	fExecThread->PostMessage(SV_LAUNCH);
	return B_OK;
}


status_t
ShellView::Exec(BString command, BString execDir)
{
	SetCommand(command);
	SetExecDir(execDir);
	return Exec();
}


void
ShellView::Clear()
{
	BTextView::SetText("");
}
