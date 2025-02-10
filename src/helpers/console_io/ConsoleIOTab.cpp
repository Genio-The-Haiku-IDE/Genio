/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConsoleIOTab.h"
#include <Catalog.h>
#include <Looper.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConsoleIOView"

ConsoleIOTab::ConsoleIOTab(const char* name):TerminalTab()
{
	SetName(name);//B_TRANSLATE("Console I/O"));
	SetInitialCommand("/bin/sh -c \"sleep 1000\"");
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
	Looper()->PostMessage(&exec, fTermView->ChildAt(0)->ChildAt(0));

	//BMessage*
	//argv[0] = strdup("/bin/sh");
	//argv[1] = strdup("-c");
	//argv[2] = strdup(cmd.String());
	return B_OK;
}


