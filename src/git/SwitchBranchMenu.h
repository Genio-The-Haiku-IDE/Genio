/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Parts are taken from the TemplatesMenu class from Haiku source code (Tracker) under the
 * Open Tracker License
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */

#pragma once

#include <Menu.h>

class SwitchBranchMenu : public BMenu {
public:
							SwitchBranchMenu(BHandler *target, const char* label,
												BMessage *message,
												const char *projectPath = nullptr);
	virtual 				~SwitchBranchMenu();

	virtual void 			AttachedToWindow();
	virtual void 			DetachedFromWindow();
	virtual void 			MessageReceived(BMessage *message);

	virtual status_t 		SetTargetForItems(BHandler* target);

private:
	bool 					_BuildMenu(const BString& projectPath);

	BHandler* 				fTarget;
	BMessage*				fMessage;
	BString					fProjectPath;
};
