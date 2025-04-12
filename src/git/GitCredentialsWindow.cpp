/*
 * Copyright Hrishikesh Hiraskar <hrishihiraskar@gmail.com>
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
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


GitCredentialsWindow::GitCredentialsWindow(const char* title, bool user, bool password)
	:
	BWindow(BRect(0, 0, 300, 150), title,
			B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_CLOSABLE),
	fUsername(nullptr),
	fPassword(nullptr),
	fUsernameString(nullptr),
	fPasswordString(nullptr)
{
	fUsername = new BTextControl(B_TRANSLATE("Username:"), "", NULL);
	if (password) {
		fPassword = new BTextControl(B_TRANSLATE("Password:"), "", NULL);
		fPassword->TextView()->HideTyping(true);
	}
	BButton* fOK = new BButton("ok", B_TRANSLATE("OK"),
			new BMessage(kCredOK));
	BButton* fCancel = new BButton("cancel", B_TRANSLATE("Cancel"),
			new BMessage(kCredCancel));

	if (password) {
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
	} else {
		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.SetInsets(B_USE_WINDOW_INSETS)
			.AddGrid()
				.AddTextControl(fUsername, 0, 0)
			.End()
			.AddGroup(B_HORIZONTAL)
				.AddGlue()
				.Add(fCancel)
				.Add(fOK)
			.End();
	}
	CenterOnScreen();
	Show();
}


void
GitCredentialsWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kCredOK:
			fUsernameString->SetTo(fUsername->Text());
			if (fPassword != nullptr && fPasswordString != nullptr)
				fPasswordString->SetTo(fPassword->Text());
			Quit();
			break;
		case kCredCancel:
			fUsernameString->SetTo("");
			if (fPassword != nullptr && fPasswordString != nullptr)
				fPasswordString->SetTo("");
			Quit();
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


/* static */
thread_id
GitCredentialsWindow::OpenCredentialsWindow(const char* title, BString& username)
{
	GitCredentialsWindow* window = new GitCredentialsWindow(title, true);
	window->fUsernameString = &username;
	return window->Thread();
}


/* static */
thread_id
GitCredentialsWindow::OpenCredentialsWindow(const char* title, BString& username,
													BString& password)
{
	GitCredentialsWindow* window = new GitCredentialsWindow(title, true, true);
	window->fUsernameString = &username;
	window->fPasswordString = &password;
	return window->Thread();
}


int
GitCredentialsWindow::authentication_callback(git_cred** out, const char* url,
									const char* username_from_url,
									unsigned int allowed_types,
									void* payload)
{
	if (Logger::IsDebugEnabled()) {
		if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT)
			printf("allowed_types: GIT_CREDENTIAL_USERPASS_PLAINTEXT\n");
		if (allowed_types & GIT_CREDENTIAL_SSH_KEY)
			printf("allowed_types: GIT_CREDENTIAL_SSH_KEY\n");
		if (allowed_types & GIT_CREDENTIAL_SSH_CUSTOM)
			printf("allowed_types: GIT_CREDENTIAL_SSH_CUSTOM\n");
		if (allowed_types & GIT_CREDENTIAL_DEFAULT)
			printf("allowed_types: GIT_CREDENTIAL_DEFAULT\n");
		if (allowed_types & GIT_CREDENTIAL_SSH_INTERACTIVE)
			printf("allowed_types: GIT_CREDENTIAL_SSH_INTERACTIVE\n");
		if (allowed_types & GIT_CREDENTIAL_USERNAME)
			printf("allowed_types: GIT_CREDENTIAL_USERNAME\n");
		if (allowed_types & GIT_CREDENTIAL_SSH_MEMORY)
			printf("allowed_types: GIT_CREDENTIAL_SSH_MEMORY\n");
	}

	BString username;
	if (allowed_types & GIT_CREDENTIAL_USERNAME) {
		// Ask for username
		thread_id thread = OpenCredentialsWindow(B_TRANSLATE("Git - Username"), username);
		status_t winStatus = B_OK;
		wait_for_thread(thread, &winStatus);
		if (!username.IsEmpty())
			return git_credential_username_new(out, username);
		return Genio::Git::CANCEL_CREDENTIALS;;
	}
	// TODO: this only works when using an ssh_agent.
	// TODO: if there's no key, this keeps looping
	// Allow user to specify the ssh key and eventually the ssh key passkey
	if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
		if (!BString(username_from_url).IsEmpty())
			return git_credential_ssh_key_from_agent(out, username_from_url);
	}

	BString password;
	thread_id thread = OpenCredentialsWindow(B_TRANSLATE("Git - User Credentials"),
											username, password);
	status_t winStatus = B_OK;
	wait_for_thread(thread, &winStatus);

	int error = 0;
	if (!username.IsEmpty() && !password.IsEmpty()) {
		error = git_cred_userpass_plaintext_new(out, username, password);
	}

	/**
	 * If user cancels the credentials prompt, the username is empty.
	 * Cancel the command in such case.
	 */
	if (username.IsEmpty())
		return Genio::Git::CANCEL_CREDENTIALS;

	return error;
}
