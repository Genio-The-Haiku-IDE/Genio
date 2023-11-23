/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 */

#pragma once

#include <TextControl.h>
#include <Window.h>

#include <string>
#include <vector>

#include "GAlert.h"

class BButton;
class BStringView;
class BScrollView;

class GTextAlert: public GAlert<BString> {
public:
								GTextAlert(const char *title, const char *message, const char *text);
								~GTextAlert();

	void						MessageReceived(BMessage* message);

private:
	BTextControl*				fTextControl;
	BString						fText;
	const uint32				kTextControlMessage = 'tcms';

	virtual void				Show();
	void						_InitInterface();
};