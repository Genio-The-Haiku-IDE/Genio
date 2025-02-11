/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConsoleIOTab.h"
#include "ConsoleIOThread.h"
#include <Catalog.h>
#include <Looper.h>
#include <cstdio>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConsoleIOView"

ConsoleIOTab::ConsoleIOTab(const char* name, BMessenger messenger):
	TerminalTab(), fMessenger(messenger)
{
	SetName(name);
	SetInitialCommand("/bin/sh -c \":\"");
}


void
ConsoleIOTab::Clear()
{
};

status_t
ConsoleIOTab::RunCommand(BMessage* message)
{
	//temporary big hack!

	BMessage exec(B_EXECUTE_PROPERTY);
	exec.AddSpecifier("command");
	exec.AddString("argv", "/bin/sh");
	exec.AddString("argv", "-c");
	exec.AddString("argv", message->GetString("cmd", "echo error"));
	exec.AddBool("clear", true);
	fContextMessage = *message;
	Looper()->PostMessage(&exec, fTermView->ChildAt(0)->ChildAt(0));
	return B_OK;
}


void
ConsoleIOTab::NotifyCommandQuit(bool exitNormal, int exitStatus)
{
	status_t status = exitNormal ? ( exitStatus == 0 ? B_OK : B_ERROR) : B_ERROR;

	if(fContextMessage.IsEmpty() == false) {
		// printf("NotifyCommandQuit: "); fContextMessage.PrintToStream();
		fContextMessage.what = CONSOLEIOTHREAD_EXIT;
		fContextMessage.AddInt32("status", status);
		fMessenger.SendMessage(&fContextMessage);
		fContextMessage.MakeEmpty();
	}
}