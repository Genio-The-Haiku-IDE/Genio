/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "TerminalManager.h"

#include <Box.h>
#include <Path.h>
#include <StringView.h>

#include "argv_split.h"
#include "Log.h"
#include "Utils.h"


TerminalManager::TerminalManager()
	:
	fId(-1)
{
	BPath genioPath = GetNearbyDataDirectory();
	if (genioPath.Append("genio_terminal_addon") == B_OK) {
		fId = load_add_on(genioPath.Path());
		if (fId < 0)
			LogError("Can't load genio_terminal_addon (%s): %s\n",
				genioPath.Path(), ::strerror(fId));
	}
	LogInfo("TerminalManager, loading addon at [%s] -> %d\n", genioPath.Path(), fId);
}

/* static */
TerminalManager&
TerminalManager::GetInstance()
{
	static TerminalManager manager;
	return manager;
}

/* static */
void	  
TerminalManager::GetThemes(BMessage* themes)
{
	TerminalManager& manager = GetInstance();
	if (manager.IsValid()) {
 		void (*_GetThemes)(BMessage*);
		status_t err = get_image_symbol(manager.fId, "GetThemes", B_SYMBOL_TYPE_ANY, (void**)&_GetThemes);
		if (err != B_OK) {
			LogError("Can't find GetThemes in terminal addon\n");
			return;
		}
		_GetThemes(themes);
	}
}



/* static */
BView*
TerminalManager::CreateNewTerminal(BRect frame, BMessenger listener, BString command, BString theme)
{
	TerminalManager& manager = GetInstance();
	BView* view = nullptr;
	if (manager.IsValid()) {
		BMessage message;
		message.AddString("class", "GenioTermView");
		message.AddBool("use_rect", true);
		message.AddString("theme", theme);
		if (!command.IsEmpty()) {
			argv_split parser;
			parser.parse(command.String());
			int32 argc = parser.getArguments().size();
			for (int32 i = 0; i < argc; i++) {
				message.AddString("argv", parser.argv()[i]);
			}
		}
		message.AddMessenger("listener", listener);
		message.AddRect("_frame", frame);
		view  = dynamic_cast<BView *>(instantiate_object(&message, &manager.fId));
	}

	if (view == nullptr) {
		view = new BStringView("info", " Can't load terminal! ");
	}
	return view;
}
