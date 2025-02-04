/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "TerminalTab.h"
#include "TerminalManager.h"

TerminalTab::TerminalTab():BView("Terminal", B_FRAME_EVENTS)
{
	SetExplicitMinSize(BSize(100,100));
	SetResizingMode(B_FOLLOW_ALL);
	fTermView = TerminalManager::CreateNewTerminal(BRect(100,100));
	fTermView->SetResizingMode(B_FOLLOW_NONE);
	AddChild(fTermView);
}


void
TerminalTab::FrameResized(float w, float h)
{
	fTermView->ResizeTo(w, h);
	BView::FrameResized(w, h);
}
