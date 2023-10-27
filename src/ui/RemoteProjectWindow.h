/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <BarberPole.h>

#include "GitRepository.h"
#include "PathBox.h"
#include "Task.h"

using namespace Genio::Task;
using namespace Genio::Git;

enum {
	kDoClone,
	kCancel,
	kFinished,
	kProgress,
	kUrlModified,
	kClose
};

class BCardLayout;
class BStatusBar;
class BStringView;
class RemoteProjectWindow : public BWindow {
public:
								RemoteProjectWindow(BString repo, BString dirPath,
												const BMessenger target);
	virtual void				MessageReceived(BMessage*);
private:
	BTextControl* 				fURL;
	PathBox* 					fPathBox;
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
	BStringView*				fDestDirLabel;
	BTextControl*				fDestDir;

	shared_ptr<Task<BPath>>		fCurrentTask;

	void						_OpenProject(const path& localPath);
	void						_ResetControls();
	BString						_ExtractRepositoryName(BString url);
	void						_SetBusy();
	void						_SetIdle();
	void						_SetProgress(float value, const char* text);
};
