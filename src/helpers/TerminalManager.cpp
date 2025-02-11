/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "TerminalManager.h"
#include <Box.h>
#include <Path.h>
#include <StringView.h>
#include "Utils.h"
#include "argv_split.h"

TerminalManager::TerminalManager():fId(-1)
{
	BPath genioDir;
	GetGenioDirectory(genioDir);
	genioDir.Append("../data/genio_terminal_addon");

	fId = load_add_on(genioDir.Path());

	printf("TerminalManager, loading addon at [%s] -> %d\n", genioDir.Path(), fId);
}

/*static */
BView*
TerminalManager::CreateNewTerminal(BRect frame,  BMessenger listener, BString command)
{
	static TerminalManager	manager;
	BView* view = nullptr;
	if (manager.IsValid()) {
		BMessage message;
		message.AddString("class", "GenioTermView");
		message.AddBool("use_rect", true);
		if (command.IsEmpty() == false) {
			argv_split parser;
			parser.parse(command.String());
			int32 argc = parser.getArguments().size();
			for (int32 i=0;i<argc;i++) {
				message.AddString("argv", parser.argv()[i]);
			}
		}
		message.AddMessenger("listener", listener);
		message.AddRect("_frame", frame);
		view  = dynamic_cast<BView *>(instantiate_object(&message, &manager.fId));
	}
	if (view == nullptr)
	{
		view = new BStringView("info", " Can't load terminal! ");
	}
	return view;
}