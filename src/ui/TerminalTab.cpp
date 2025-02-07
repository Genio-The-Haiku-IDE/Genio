/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "TerminalTab.h"
#include "TerminalManager.h"
#include <Messenger.h>
#include <cstdio>
#include <sys/wait.h>

TerminalTab::TerminalTab():BView("Terminal", B_FRAME_EVENTS)
{
	SetExplicitMinSize(BSize(100,100));
	SetResizingMode(B_FOLLOW_ALL);
}


void
TerminalTab::FrameResized(float w, float h)
{
	if (fTermView)
		fTermView->ResizeTo(w, h);
	BView::FrameResized(w, h);
}


void
TerminalTab::AttachedToWindow()
{
	BView::AttachedToWindow();
	fTermView = TerminalManager::CreateNewTerminal(BRect(100,100), BMessenger(this));
	fTermView->SetResizingMode(B_FOLLOW_NONE);
	AddChild(fTermView);
}

void
TerminalTab::MessageReceived(BMessage* msg)
{
	if (msg->what == 'NOTM') {
		msg->PrintToStream();
		int status = -1;
		pid_t pid = msg->GetInt32("pid", -1);

		 if (waitpid(pid, &status, 0) > 0) {
			if (WIFEXITED(status) && !WEXITSTATUS(status)) {
			  printf("/* the program terminated normally and executed successfully */\n");
			} else if (WIFEXITED(status) && WEXITSTATUS(status)) {
			  printf("/* the program terminated normally, but returned a non-zero status */\n");
			} else {
			  printf("/* the program didn't terminate normally */\n");
			}
		  } else {
			printf("/* waitpid() failed */\n");
		  }
		return;
	}
	BView::MessageReceived(msg);
}
