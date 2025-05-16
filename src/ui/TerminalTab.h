/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <GroupView.h>


class TerminalTab : public BView {
public:
				TerminalTab();
		void	FrameResized(float w, float h) override;
		void	MessageReceived(BMessage* msg) override;
		void	AttachedToWindow() override;

		void	SetInitialCommand(const char* command);

virtual	void 	NotifyCommandQuit(bool exitNormal, int exitStatus);

protected:
	BView*	fTermView;
	BString fCommand;
};
