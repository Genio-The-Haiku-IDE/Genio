/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GrepThread.h"


GrepThread::GrepThread(BMessage* cmd_message, const BMessenger& consoleTarget)
	: ConsoleIOThread(cmd_message, consoleTarget)
{
	fCurrentMessage.MakeEmpty();
	fCurrentFileName[0] = '\0';
	fNextFileName[0] = '\0';
}

// Method freely derived from TextGrep by Matthijs Hollemans
void
GrepThread::OnStdOutputLine(const BString& stdOut)
{
	stdOut.CopyInto(fLine, 0, stdOut.Length() > MAX_LINE_LEN ? MAX_LINE_LEN : stdOut.Length());
	fLine[stdOut.Length()] = '\0';

	// parse grep output
	fNextFileName[0] = '\0';

	int lineNumber = -1;
	int textPos = -1;
	sscanf(fLine, "%[^\n:]:%d:%n", fNextFileName, &lineNumber, &textPos);
	if (textPos > 0) {
		if (strcmp(fNextFileName, fCurrentFileName) != 0) {
			fTarget.SendMessage(&fCurrentMessage);
			strncpy(fCurrentFileName, fNextFileName, B_PATH_NAME_LENGTH);
			BEntry entry(fNextFileName);
			entry.GetRef(&fCurrentRef);

			fCurrentMessage.MakeEmpty();
			fCurrentMessage.what = MSG_REPORT_RESULT;
			fCurrentMessage.AddString("filename", fCurrentFileName);
		}

		char* text = &fLine[strlen(fNextFileName) + 1];
		BMessage lineMessage;
		lineMessage.what = B_REFS_RECEIVED;
		lineMessage.AddString("text", text);
		lineMessage.AddRef("refs", &fCurrentRef);
		lineMessage.AddInt32("be:line", lineNumber);
		fCurrentMessage.AddMessage("line", &lineMessage);
	}
}


void
GrepThread::ThreadExitNotification()
{
	if (fCurrentMessage.HasMessage("line"))
		fTarget.SendMessage(&fCurrentMessage);

	fCurrentMessage.MakeEmpty();
	fCurrentMessage.what = MSG_GREP_DONE;
	fTarget.SendMessage(&fCurrentMessage);
}
