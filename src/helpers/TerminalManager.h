/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <Archivable.h>
#include <View.h>

class TerminalManager {
public:
	static BView*	CreateNewTerminal(BRect frame);
private:
				TerminalManager();
		bool	IsValid() { return fId > -1; }
	image_id	fId;
};


