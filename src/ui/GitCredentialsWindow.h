/*
 * Copyright 2018, Hrishikesh Hiraskar <hrishihiraskar@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef _CREDENTIALS_WINDOW_H_
#define _CREDENTIALS_WINDOW_H_

#include <sys/time.h>

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
							GitCredentialsWindow(const char* username,
												const char* password);
	virtual void			MessageReceived(BMessage* message);
private:
	/**
	 * The text control for username.
	 */
	BTextControl* 			fUsername;
	/**
	 * The text control for password.
	 */
	BTextControl* 			fPassword;
	
	BString					fUsernameString;
	BString					fPasswordString;

};

#endif
