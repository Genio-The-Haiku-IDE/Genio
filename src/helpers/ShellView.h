/*
 * Copyright 2014 Augustin Cavalier <waddlesplash>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SHELLVIEW_H
#define SHELLVIEW_H

#include <Messenger.h>
#include <TextView.h>
#include <ScrollView.h>
#include <String.h>

// Predefinitions
class ExecThread;

class ShellView : public BTextView {
public:
	// 'what' codes that are used internally by this class
	enum WhatCodes {
		SV_LAUNCH 	= 'svLA',
		SV_DATA	 	= 'svDA',
		SV_DONE	 	= 'svDO'
	};

					ShellView(const char* name, const BMessenger& messenger, uint32 what);
		void		MessageReceived(BMessage* msg);

		BScrollView*	ScrollView() { return fScrollView; }

		status_t	SetCommand(BString command);
		status_t	SetExecDir(BString execDir);
		status_t	Exec();
		status_t	Exec(BString command, BString execDir = ".");
		void 		Clear();

private:
		BString			fCommand;
		BString			fExecDir;

		ExecThread*		fExecThread;
		BScrollView*	fScrollView;

		BMessenger		fMessenger;
		uint32			fNotifyFinishedWhat;
};

#endif // SHELLVIEW_H
