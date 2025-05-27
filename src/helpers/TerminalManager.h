/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <Messenger.h>
#include <String.h>

#include <image.h>

class BView;
class TerminalManager {
public:
	static BView* CreateNewTerminal(BRect frame, BMessenger listener, BString command = "");
private:
			TerminalManager();
	bool	IsValid() const { return fId > -1; }
	image_id fId;
};
