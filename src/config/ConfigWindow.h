/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ConfigWindow_H
#define ConfigWindow_H

#include "GMessage.h"

#include <Window.h>

class BButton;
class BCardView;
class BOutlineListView;
class BView;
class ConfigManager;
class BOptionPopUp;

class ConfigWindow : public BWindow {
public:
		ConfigWindow(ConfigManager& configManager);

		void MessageReceived(BMessage* message);

		BView* MakeViewFor(const char* groupName, GMessage& config);
		BView* MakeControlFor(GMessage& config);
		BView* MakeNoteView(GMessage& config);

		bool			QuitRequested();

private:

	ConfigManager& fConfigManager;

	BView*	_Init();
	void 	_PopulateListView();

	template<typename T>
	BOptionPopUp*	_CreatePopUp(GMessage& config);

	BOutlineListView* 	fGroupList;
	BCardView* 	fCardView;
	BButton* fDefaultsButton;
	bool	fShowHidden;
};


#endif // ConfigWindow_H
