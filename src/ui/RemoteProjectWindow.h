/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Based on TrackGit (https://github.com/HaikuArchives/TrackGit)
 * Original author: Hrishikesh Hiraskar <hrishihiraskar@gmail.com> 
 * Copyright Hrishikesh Hiraskar and other HaikuArchives contributors (see GitHub repo for details)
 */

#pragma once

#include <InterfaceKit.h>

#include "GitRepository.h"
#include "PathBox.h"

enum {
	kDoClone,
	kCancel,
	kFinished
};

class RemoteProjectWindow : public BWindow {
public:
							RemoteProjectWindow(BString repo, BString dirPath, 
												const BMessenger target);
	virtual void			MessageReceived(BMessage*);
private:
	BTextControl* 			fURL;
	PathBox* 				fPathBox;
	bool					fIsCloning;
	const BMessenger		fTarget;
	BButton* 				fClone;
	BButton* 				fCancel;
};


class OpenRemoteProgressWindow : public BWindow
{
	RemoteProjectWindow*	fRemoteProjectWindow;
	BTextView*				fTextView;
	BStatusBar*				fProgressBar;
	public:
							OpenRemoteProgressWindow(RemoteProjectWindow*);
	void					SetText(const char*);
	void					SetProgress(float);
	virtual void			MessageReceived(BMessage*);
};