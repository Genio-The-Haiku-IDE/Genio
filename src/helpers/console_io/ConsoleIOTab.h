/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <Messenger.h>
#include "TerminalTab.h"

class ConsoleIOTab : public TerminalTab {
public:
	ConsoleIOTab(const char* name, BMessenger messenger);

	void 		Clear();
	status_t	RunCommand(BMessage*);
	status_t	Stop();
	void		SetMessenger(BMessenger messenger) { fMessenger = messenger; }

	void 	NotifyCommandQuit(bool exitNormal, int exitStatus) override;
private:

	BView*		_FindTarget();
	BMessage	fContextMessage;
	BMessenger  fMessenger;

};


