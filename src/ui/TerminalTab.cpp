/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "TerminalTab.h"
#include "TerminalManager.h"

TerminalTab::TerminalTab():BGroupView()
{
	fTermView = TerminalManager::CreateNewTerminal(Bounds());
	//FIXME:
	SetName("Terminal");
	AddChild(fTermView);
}

void
TerminalTab::AttachedToWindow()
{
	//fTermView->ResizeTo(Bounds().Width(), Bounds().Height());
}