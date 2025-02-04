/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <GroupView.h>
#include <SupportDefs.h>
#include <View.h>

class TerminalTab : public BView {
public:
				TerminalTab();
		void	FrameResized(float w, float h) override;
private:
	BView*	fTermView;

};


