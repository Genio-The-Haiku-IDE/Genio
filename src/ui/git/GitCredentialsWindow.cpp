/*
 * Copyright Hrishikesh Hiraskar 
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <cstdio>
#include <cstring>

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <TextControl.h>

#include "GitCredentialsWindow.h"
#include "GitRepository.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GitCredentialsWindow"

GitCredentialsWindow::GitCredentialsWindow(BString &username, BString &password)
	:
	BWindow(BRect(0, 0, 300, 150), B_TRANSLATE("Git - User Credentials"),
			B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_CLOSABLE)
{
	fUsernameString = &username;
	fPasswordString = &password;

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
			fUsernameString->SetTo(fUsername->Text());
			fPasswordString->SetTo(fPassword->Text());
			Quit();
			break;
		case kCredCancel:
			fUsernameString->SetTo("");
			fPasswordString->SetTo("");
			Quit();
			break;
		// default:
			// BWindow::MessageReceived(msg);
	}
}

int
GitCredentialsWindow::authentication_callback(git_cred** out, const char* url,
									const char* username_from_url,
									unsigned int allowed_types,
									void* payload)
{
	BString username, password;
	int error = 0;

	// strcpy(username, username_from_url);

	GitCredentialsWindow* window = new GitCredentialsWindow(username, password);

	thread_id thread = window->Thread();
	status_t win_status = B_OK;
	wait_for_thread(thread, &win_status);

	if (strlen(username) != 0 && strlen(password) != 0) {
		if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
			error = git_credential_ssh_key_from_agent(out, username);
		} else {
			error = git_cred_userpass_plaintext_new(out, username, password);
		}
	}

	/**
	 * If user cancels the credentials prompt, the username is empty.
	 * Cancel the command in such case.
	 */
	if (strlen(username) == 0)
		return Genio::Git::CANCEL_CREDENTIALS;

	return error;
}
