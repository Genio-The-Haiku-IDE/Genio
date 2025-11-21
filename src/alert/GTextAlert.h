/*
 * Copyright The Genio Contributors
 * All rights reserved. Distributed under the terms of the MIT license.
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
								GTextAlert(const char *title, const char *message,
									const char *text, bool checkIfTextHasChanged = true);

	void						MessageReceived(BMessage* message);

private:
								~GTextAlert();

	BTextControl*				fTextControl;
	BString						fText;
	bool						fCheckIfTextHasChanged;

	virtual void				Show();
	void						_InitInterface();
	void						_CheckTextModified();
};