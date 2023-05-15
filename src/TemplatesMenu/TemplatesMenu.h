/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 * Parts are taken from the TemplatesMenu class from Haiku source code (Tracker) under the 
 * Open Tracker Licence 
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */
 
#ifndef _TEMPLATES_MENU_H
#define _TEMPLATES_MENU_H


#include <Menu.h>

#define DIR_FILETYPE "application/x-vnd.Be-directory"
#define FILE_FILETYPE "application/octet-stream"

extern const char* 		kTemplatesMenuName;
extern const char* 		kTemplatesDirectory;

class TemplatesMenu : public BMenu {
public:
						TemplatesMenu(const BHandler* target, const char* label, uint32 command);
	virtual 			~TemplatesMenu();

	virtual void 		AttachedToWindow();

	virtual status_t 	SetTargetForItems(const BHandler* target);

	void 				UpdateMenuState();

private:
	bool 				_BuildMenu(bool addItems = true);

	const BHandler* 	fTarget;
	BMenuItem* 			fOpenItem;
	uint32				fCommand;
};

#endif	// _TEMPLATES_MENU_H
