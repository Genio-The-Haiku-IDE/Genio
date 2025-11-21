/*
 * Copyright The Genio Contributors
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Parts taken from the TemplatesMenu class from Haiku (Tracker) under the
 * Open Tracker Licence
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */


#pragma once

#include <Menu.h>
#include <functional>

#include "ExtensionManager.h"


class ToolsMenu : public BMenu {
public:

							ToolsMenu(const char* label, uint32 command, BHandler *target);
	virtual 				~ToolsMenu();

	virtual void 			AttachedToWindow();

	void 					SetTarget(BHandler *target) { fTarget = target; }

private:
	void 					_BuildMenu();

	uint32					fCommand;
	BHandler*				fTarget;
};