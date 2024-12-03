/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <GroupView.h>
#include "TabManager.h"

class GenioTabView : public BGroupView, public TabManager {
public:
		GenioTabView(BHandler* handler);
		void MessageReceived(BMessage* message);
		void	AttachedToWindow();
private:

};


