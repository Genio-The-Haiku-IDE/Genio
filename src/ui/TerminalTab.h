/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <GroupView.h>
#include <SupportDefs.h>


class TerminalTab : public BGroupView {
public:
				TerminalTab();
		void	AttachedToWindow();
private:
	BView*	fTermView;

};


