/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once


#include <Window.h>

#include <memory>

#include "Task.h"

using namespace Genio::Task;

enum {
	kDoClone,
	kCancel,
	kFinished,
	kProgress,
	kUrlModified,
	kClose
};

class BarberPole;
class BButton;
class BCardLayout;
class BFilePanel;
class BStatusBar;
class BString;
class BStringView;
class BTextControl;
class RemoteProjectWindow : public BWindow {
public:
								RemoteProjectWindow(BString repo, BString dirPath,
												const BMessenger target);
	virtual void				MessageReceived(BMessage*);
private:
	BTextControl* 				fURL;
	BTextControl* 				fPathBox;
	BButton*					fBrowseButton;
	bool						fIsCloning;
	const BMessenger			fTarget;
	BButton* 					fClone;
	BButton* 					fCancel;
	BButton* 					fClose;
	BStatusBar*					fProgressBar;
	BarberPole*					fBarberPole;
	BCardLayout*				fProgressLayout;
	BCardLayout*				fButtonsLayout;
	BView*						fProgressView;
	BView*						fButtonsView;
	BStringView*				fStatusText;
	BTextControl*				fDestDir;
	BFilePanel*					fFilePanel;

	shared_ptr<Task<BPath>>		fCurrentTask;

	void						_OpenProject(const BString& localPath);
	void						_ResetControls();
	BString						_ExtractRepositoryName(const BString& url);
	void						_SetBusy();
	void						_SetIdle();
	void						_SetProgress(float value, const char* text);
};
