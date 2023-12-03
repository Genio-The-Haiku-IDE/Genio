/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 */

#pragma once

#include <TextControl.h>

#include "GAlert.h"

class BButton;
class BStringView;

enum TextAlertMessages {
	MsgTextChanged = 'mtch'
};

class GTextAlert: public GAlert<BString> {
public:
								GTextAlert(const char *title, const char *message, const char *text);

	void						MessageReceived(BMessage* message);

private:
								~GTextAlert();

	BTextControl*				fTextControl;
	BString						fText;

	virtual void				Show();
	void						_InitInterface();
	void						_CheckTextModified();
};