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
	//TODO!
};


status_t
ConsoleIOTab::Stop()
{
	BMessage stop;
	BString cmd(":");
	cmd << "\n" << _BannerCommand(fContextMessage.GetString("banner_claim", "command"), "STOPPED   ");
	stop.AddString("cmd", cmd);
	stop.AddBool("internalStop", true);
	return RunCommand(&stop, false, false);
};


status_t
ConsoleIOTab::RunCommand(BMessage* message, bool clean, bool notifyMessage)
{
	//temporary big hack!
	BView*	target = _FindTarget();
	if (target == nullptr)
		return B_ERROR;

	BString cmd;
	if (notifyMessage) {
		cmd << _BannerCommand(message->GetString("banner_claim", "command"), "started   ") << "\n";
	}
	cmd << message->GetString("cmd", "echo error");
	if (notifyMessage) {
		cmd << "\n" << _BannerCommand(message->GetString("banner_claim", "command"), "ended     ");
	}
	BMessage exec(B_EXECUTE_PROPERTY);
	exec.AddSpecifier("command");
	exec.AddString("argv", "/bin/sh");
	exec.AddString("argv", "-c");
	exec.AddString("argv", cmd);
	exec.AddBool("clear", clean);
	if (notifyMessage)
		fContextMessage = *message;

	return Looper()->PostMessage(&exec, target);
}

BString
ConsoleIOTab::_BannerCommand(BString claim, BString status)
{
/*	if (!gCFG["console_banner"])
		return "";*/

	BString banner("echo '");
	banner  << "--------------------------------"
			<< "   "
			<< claim
			<< " "
			<< status
			<< "--------------------------------";
	banner << "'";
	return banner;
}

void
ConsoleIOTab::NotifyCommandQuit(bool exitNormal, int exitStatus)
{
	status_t status = exitNormal ? ( exitStatus == 0 ? B_OK : B_ERROR) : B_ERROR;

	if(fContextMessage.IsEmpty() == false) {
		BMessage notification = fContextMessage;
		fContextMessage.MakeEmpty();

		notification.what = CONSOLEIOTHREAD_EXIT;
		notification.AddInt32("status", status);
		notification.what = CONSOLEIOTHREAD_EXIT;
		notification.AddInt32("status", status);
		fMessenger.SendMessage(&notification);
	}
}

BView*
ConsoleIOTab::_FindTarget()
{
	//We need a more deterministic way to find the view!
	if (fTermView && fTermView->ChildAt(0) && fTermView->ChildAt(0)->ChildAt(0))
		return fTermView->ChildAt(0)->ChildAt(0);

	return nullptr;
}