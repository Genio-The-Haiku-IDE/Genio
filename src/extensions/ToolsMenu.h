/*
 * Copyright 2023, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6@disroot.org>
 *
 * Parts taken from the TemplatesMenu class from Haiku (Tracker) under the
 * Open Tracker Licence
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */


#pragma once

#include <Menu.h>

#include "ExtensionManager.h"


class ToolsMenu : public BMenu {
public:

							ToolsMenu(const char* label, ExtensionContext& context, uint32 command,
								BHandler *target);
	virtual 				~ToolsMenu();

	virtual void 			AttachedToWindow();

private:
	void 					_BuildMenu();

	ExtensionContext*		fContext;
	uint32					fCommand;
	BHandler*				fTarget;
};