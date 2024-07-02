/*
 * Copyright 2024, My Name <my@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include "ScintillaView.h"
#include <Messenger.h>
#include <cstdio>
#include <MessageFilter.h>

enum KeyTextViewMessages {
	kKTVInputBuffer = 'flus'
};

class KeyTextViewScintilla : public BScintillaView {
public:
		KeyTextViewScintilla(const char* name, const BMessenger& msgr);
		void	ClearBuffer() { fBuffer = ""; }

		void	AttachedToWindow();

		void	EnableInput(bool enable);

		void	ClearAll();
		void	Append(const BString& text);


		filter_result	BeforeMessageReceived(BMessage* msg, BView* scintillaView);
private:
		filter_result	BeforeKeyDown(BMessage* msg, BView* scintillaView);

		void				NotificationReceived(SCNotification* n);

	BMessenger fTarget;
	BString	   fBuffer; //maybe can be better. (see BasicTerminalBuffer in Terminal)
	bool	   fEnableInput;
	Sci_Position fCaretPosition;
};


