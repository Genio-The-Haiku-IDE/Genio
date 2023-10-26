/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ConfigWindow_H
#define ConfigWindow_H

#include "GMessage.h"

#include <Box.h>
#include <LayoutBuilder.h>
#include <OutlineListView.h>
#include <Window.h>

class BButton;
class ConfigManager;
class ConfigWindow : public BWindow {
public:
		ConfigWindow(ConfigManager& configManager);

		void MessageReceived(BMessage* message);

		BView* MakeViewFor(const char* groupName, GMessage& config);
		BView* MakeSelfHostingViewFor(GMessage& config);
		BView* MakeViewFor(GMessage& config);

private:

	ConfigManager& fConfigManager;

	BView*	_Init();
	void _PopulateListView();

	BOutlineListView* 	fGroupList;
	BCardView* 	fCardView;
	BButton* fDefaultsButton;
};


#endif // ConfigWindow_H
