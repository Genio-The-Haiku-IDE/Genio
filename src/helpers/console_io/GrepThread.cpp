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
}



void
GrepThread::OnStdOutputLine(const BString& stdOut)
{
	if (stdOut.Length() > MAX_LINE_LEN) 
		return;
		
	stdOut.CopyInto(fLine, 0, stdOut.Length());
	fLine[stdOut.Length()] = '\0';
	//printf("GREP [%s]\n", fLine);
	
	// parse grep output
	char fileName[B_PATH_NAME_LENGTH];

	int lineNumber = -1;
	int textPos = -1;
	sscanf(fLine, "%[^\n:]:%d:%n", fileName, &lineNumber, &textPos);
	// printf("sscanf(\"%s\") -> %s %d %d\n", line, fileName,
	//		lineNumber, textPos);
	//printf("fileName [%s]\n", fileName);
	//printf("lineNumber [%d]\n", lineNumber);
	//printf("textPos [%d]\n", textPos);
	if (textPos > 0) {
		if (strcmp(fileName, fCurrentFileName) != 0) {
			
			fTarget.SendMessage(&fCurrentMessage);
			strncpy(fCurrentFileName, fileName,
						sizeof(fCurrentFileName));
						
			BEntry entry(fileName);
			entry.GetRef(&fCurrentRef);
			
			fCurrentMessage.MakeEmpty();
			fCurrentMessage.what = MSG_REPORT_RESULT;
			fCurrentMessage.AddString("filename", fCurrentFileName);
		}

		char* text = &fLine[strlen(fileName) + 1];
		BMessage lineMessage;
		lineMessage.what = B_REFS_RECEIVED;
		lineMessage.AddString("text", text);
		lineMessage.AddRef("refs", &fCurrentRef);
		lineMessage.AddInt32("be:line", lineNumber); //+1??
		//lineMessage.AddInt32("lsp:character", textPos);
		fCurrentMessage.AddMessage("line", &lineMessage);
	}
}

void
GrepThread::OnThreadShutdown()
{
	if (fCurrentMessage.HasMessage("line"))
		fTarget.SendMessage(&fCurrentMessage);
	
	fCurrentMessage.MakeEmpty();
	fCurrentMessage.what = MSG_GREP_DONE;
	fTarget.SendMessage(&fCurrentMessage);	
}
