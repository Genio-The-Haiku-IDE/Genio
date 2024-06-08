/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include "GMessage.h"

#include <Window.h>

class BButton;
class BCardView;
class BOptionPopUp;
class BOutlineListView;
class BView;
class ConfigManager;

class ConfigWindow : public BWindow {
public:
	ConfigWindow(ConfigManager& configManager);

	void MessageReceived(BMessage* message);

	BView* MakeViewFor(const char* groupName, GMessage& config);
	BView* MakeControlFor(GMessage& config);
	BView* MakeNoteView(GMessage& config);

	bool QuitRequested();

private:
	ConfigManager&		fConfigManager;

	BView*	_Init();
	void 	_PopulateListView();

	template<typename T, typename POPUP>
	BOptionPopUp*	_CreatePopUp(GMessage& config);

	BOutlineListView* 	fGroupList;
	BCardView*			fCardView;
	BButton*			fDefaultsButton;
	bool				fShowHidden;
};

