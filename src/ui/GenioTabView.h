/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <GroupView.h>
#include <map>
#include "TabManager.h"

typedef uint32 tab_key;

class GenioTabView : public BGroupView, public TabManager {
public:
				GenioTabView(BHandler* handler);
		void 	MessageReceived(BMessage* message);
		void	AttachedToWindow();

		void	AddTab(tab_key id, BView* view);

		void	SetTabLabel(BView* view, const char* label);

		void	SelectTab(tab_key id);

		//temporary:
		float				TabHeight() const;

private:

	std::map<tab_key, BView*>	fTabMap;

};


