/*
 * Copyright Hrishikesh Hiraskar <hrishihiraskar@gmail.com>
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "GitCredentialsWindow.h"

#include <stdio.h>
#include <string.h>

#include <AppKit.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <SupportKit.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GitCredentialsWindow"

GitCredentialsWindow::GitCredentialsWindow(char* usernamePtr, char* passwordPtr)
	:
	BWindow(BRect(0, 0, 300, 150), B_TRANSLATE("Git - User Credentials"),
			B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_CLOSABLE)
{
	fUsernamePtr = usernamePtr;
	fPasswordPtr = passwordPtr;

	fUsername = new BTextControl(B_TRANSLATE("Username:"), "", NULL);
	fPassword = new BTextControl(B_TRANSLATE("Password:"), "", NULL);
	fPassword->TextView()->HideTyping(true);
	BButton* fOK = new BButton("ok", B_TRANSLATE("OK"),
			new BMessage(kCredOK));
	BButton* fCancel = new BButton("cancel", B_TRANSLATE("Cancel"),
			new BMessage(kCredCancel));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.AddGrid()
		.AddTextControl(fUsername, 0, 0)
		.AddTextControl(fPassword, 0, 1)
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fCancel)
			.Add(fOK)
			.End();

	CenterOnScreen();
	Show();
}


void
GitCredentialsWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kCredOK:
			strcpy(fUsernamePtr, fUsername->Text());
			strcpy(fPasswordPtr, fPassword->Text());
			Quit();
			break;
		case kCredCancel:
			strcpy(fUsernamePtr, "");
			strcpy(fPasswordPtr, "");
			Quit();
			break;
		// default:
			// BWindow::MessageReceived(msg);
	}
}
