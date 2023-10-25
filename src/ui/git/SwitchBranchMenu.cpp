/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 * Parts are taken from the TemplatesMenu class from Haiku (Tracker) under the 
 * Open Tracker Licence 
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */

#include "SwitchBranchMenu.h"

#include <Application.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Mime.h>
#include <MimeType.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Query.h>
#include <Roster.h>

#include <cstdio>
#include <list>

#include "MimeType.h"
#include "GenioWindowMessages.h"
#include "GenioWindow.h"
#include "GitRepository.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SwitchBranchMenu"


SwitchBranchMenu::SwitchBranchMenu(BHandler *target, const char* label, 
									BMessage *message)
	:
	BMenu(label),
	fTarget(target),
	fMessage(message),
	fActiveProjectPath(nullptr),
	fDetectActiveProject(true)
{
	SetRadioMode(true);
}


SwitchBranchMenu::SwitchBranchMenu(BHandler *target, const char* label, 
									BMessage *message, const char *projectPath)
	:
	BMenu(label),
	fTarget(target),
	fMessage(message),
	fActiveProjectPath(projectPath),
	fDetectActiveProject(false)
{
	SetRadioMode(true);
}


SwitchBranchMenu::~SwitchBranchMenu()
{
}


void
SwitchBranchMenu::AttachedToWindow()
{
	if (fDetectActiveProject) {
		GenioWindow *window = reinterpret_cast<GenioWindow *>(fTarget);
		fActiveProjectPath = window->GetActiveProject()->Path().String();
	}
	_BuildMenu();
	BMenu::AttachedToWindow();
	SetTargetForItems(fTarget);
}

void 
SwitchBranchMenu::DetachedFromWindow()
{
}

void
SwitchBranchMenu::MessageReceived(BMessage *message)
{
}


status_t
SwitchBranchMenu::SetTargetForItems(BHandler* target)
{
	return BMenu::SetTargetForItems(target);
}


void
SwitchBranchMenu::UpdateMenuState()
{
	_BuildMenu();
}


bool
SwitchBranchMenu::_BuildMenu()
{
	// clear everything...
	int32 count = CountItems();
	while (count--)
		delete RemoveItem((int32)0);
	
	if (fActiveProjectPath) {
		Genio::Git::GitRepository repo(fActiveProjectPath);
		auto branches = repo.GetBranches();
		auto current_branch = repo.GetCurrentBranch();
		for(auto &branch : branches) {
			BMessage *message = new BMessage(fMessage->what);
			message->AddString("branch", branch);
			auto item = new BMenuItem(branch, message);
			AddItem(item);
			if (branch == current_branch)
				item->SetMarked(true);
		}
	}
	return count > 0;
}
