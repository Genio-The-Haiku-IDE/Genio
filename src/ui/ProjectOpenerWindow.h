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
class ProjectOpenerWindow : public BWindow {
public:
	ProjectOpenerWindow(const entry_ref*, const BMessenger& messenger, bool activate);
	
private:
	void	_OpenProject(const entry_ref* ref, bool activate);

	const BMessenger			fTarget;
	BButton* 					fCancel;
	BStatusBar*					fProgressBar;
	BarberPole*					fBarberPole;
	BCardLayout*				fProgressLayout;
	BView*						fProgressView;
	BStringView*				fStatusText;
};
