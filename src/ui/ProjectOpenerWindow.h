/*
 * Copyright 2025, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <Window.h>

#include <Messenger.h>


class BarberPole;
class BButton;
class BCardLayout;
class BStatusBar;
class BString;
class BStringView;
class ProjectFolder;
class ProjectOpenerWindow : public BWindow {
public:
	ProjectOpenerWindow(const BMessenger& messenger);
	
	void MessageReceived(BMessage* message) override;
private:
	void _MoveAndResize();
	const BMessenger			fTarget;
};
