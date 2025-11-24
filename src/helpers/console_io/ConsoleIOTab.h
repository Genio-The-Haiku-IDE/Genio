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
	ConsoleIOTab(const char* name, BMessenger messenger, BString theme);

	void 		Clear();
	status_t	RunCommand(BMessage* , bool clean = true, bool notifyMessage = true);
	status_t	Stop();
	void		SetMessenger(BMessenger messenger) { fMessenger = messenger; }

	void 		NotifyCommandQuit(bool exitNormal, int exitStatus) override;
	void		SetTheme(BString theme);

private:

	BView*		_FindTarget();
	BString		_BannerCommand(BString claim, BString status, bool ending);
	BMessage	fContextMessage;
	BMessenger  fMessenger;

};


