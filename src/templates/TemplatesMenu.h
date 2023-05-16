/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Parts are taken from the TemplatesMenu class from Haiku source code (Tracker) under the 
 * Open Tracker Licence 
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */
 
#ifndef _TEMPLATES_MENU_H
#define _TEMPLATES_MENU_H


#include <Menu.h>

#include <list>
#include <string>

#define DIR_FILETYPE "application/x-vnd.Be-directory"
#define FILE_FILETYPE "application/octet-stream"

class TemplatesMenu : public BMenu {
public:

	enum ViewMode {
		FILE_VIEW_MODE,
		DIRECTORY_VIEW_MODE,
		SHOW_ALL_VIEW_MODE
	};

							TemplatesMenu(const BHandler *target, const char* label, 
											BMessage *message, BMessage *show_template_message,
											const BString& defaultDirectory, 
											const BString& userDirectory,
											ViewMode mode = FILE_VIEW_MODE, 
											bool showNewDirectory = true);
	virtual 				~TemplatesMenu();

	virtual void 			AttachedToWindow();

	virtual status_t 		SetTargetForItems(const BHandler* target);

	void 					UpdateMenuState();
	
	void					SetViewMode(ViewMode mode) { fViewMode = mode; }
	void					ShowNewFolder(bool show) { fShowNewFolder = show; }

private:
	bool 					_BuildMenu(bool addItems = true);
	void					_BuildTemplateItems(BString directory);

	const BHandler* 		fTarget;
	BMenuItem* 				fOpenItem;
	BMessage*				fMessage;
	BMessage*				fShowTemplateMessage;
	ViewMode				fViewMode;
	const BString			fDefaultDirectory;
	const BString			fUserDirectory;
	bool					fShowNewFolder;
	bool					fShowTemplatesDirectory;
};

#endif	// _TEMPLATES_MENU_H
