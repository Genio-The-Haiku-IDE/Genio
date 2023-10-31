/*
 * Copyright 2018, Hrishikesh Hiraskar 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef _CREDENTIALS_WINDOW_H_
#define _CREDENTIALS_WINDOW_H_

#include <posix/sys/time.h>

#include <InterfaceKit.h>

enum {
	kCredOK,
	kCredCancel
};

/**
 * The Credentials Window class.
 */
class GitCredentialsWindow : public BWindow {
	/**
	 * The text control for username.
	 */
	BTextControl* 			fUsername;
	/**
	 * The text control for password.
	 */
	BTextControl* 			fPassword;
	/**
	 * The username pointer.
	 */
	char*					fUsernamePtr;
	/**
	 * The password pointer.
	 */
	char*					fPasswordPtr;
	public:
							GitCredentialsWindow(char*, char*);
	virtual void			MessageReceived(BMessage*);
};

#endif
