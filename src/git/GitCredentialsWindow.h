/*
 * Copyright 2018, Hrishikesh Hiraskar <hrishihiraskar@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <sys/time.h>

#include <git2.h>
#include <String.h>
#include <Window.h>


enum {
	kCredOK,
	kCredCancel
};

class BTextControl;
/**
 * The Credentials Window class.
 */
class GitCredentialsWindow : public BWindow {
public:
							GitCredentialsWindow(const char* title, bool username, bool password = false);

	void					MessageReceived(BMessage* message) override;

	static thread_id		OpenCredentialsWindow(const char* title, BString& username);
	static thread_id		OpenCredentialsWindow(const char* title, BString& username,
													BString& password);
	static int 				authentication_callback(git_cred** out, const char* url,
													const char* username_from_url,
													unsigned int allowed_types,
													void* payload);
private:
	/**
	 * The text control for username.
	 */
	BTextControl* 			fUsername;
	/**
	 * The text control for password.
	 */
	BTextControl* 			fPassword;

	BString*				fUsernameString;
	BString*				fPasswordString;
};
