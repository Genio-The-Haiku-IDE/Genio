/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <InterfaceKit.h>

#include "GitRepository.h"
#include "PathBox.h"
#include "Task.h"

using namespace Genio::Task;
using namespace Genio::Git;

enum {
	kDoClone,
	kCancel,
	kFinished,
	kProgress
};

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
	BStatusBar*					fProgressBar;
	
	shared_ptr<Task<BPath>>		fCurrentTask;
	
	void						_OpenProject(const path& localPath);
	void						_ResetControls();
};