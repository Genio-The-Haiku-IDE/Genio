/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
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
#include <GroupLayout.h>
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


#include "GenioApp.h"
#include "GenioWindow.h"
#include "GitRepository.h"
#include "ProjectFolder.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SwitchBranchMenu"


SwitchBranchMenu::SwitchBranchMenu(BHandler *target, const char* label,
									BMessage *message, const char *projectPath)
	:
	BMenu(label),
	fTarget(target),
	fMessage(message),
	fProjectPath(projectPath)
{
	SetRadioMode(true);
}


SwitchBranchMenu::~SwitchBranchMenu()
{
}


void
SwitchBranchMenu::AttachedToWindow()
{
	BString projectPath = fProjectPath;
	if (projectPath.IsEmpty()) {
		auto activeProject = gMainWindow->GetActiveProject();
		// GenioWindow *window = reinterpret_cast<GenioWindow *>(fTarget);
		// fActiveProjectPath = window->GetActiveProject()->Path().String();
		if (activeProject != nullptr)
			projectPath = activeProject->Path().String();
	}
	_BuildMenu(projectPath);

	// This has to be done AFTER _BuildMenu, since
	// it layouts the menu and resizes the window.
	// So we need to have the menuitems already added.
	BMenu::AttachedToWindow();

	SetTargetForItems(fTarget);
}


void
SwitchBranchMenu::DetachedFromWindow()
{
	BMenu::DetachedFromWindow();
}


void
SwitchBranchMenu::MessageReceived(BMessage *message)
{
	BMenu::MessageReceived(message);
}


status_t
SwitchBranchMenu::SetTargetForItems(BHandler* target)
{
	return BMenu::SetTargetForItems(target);
}


bool
SwitchBranchMenu::_BuildMenu(const BString& projectPath)
{
	// clear everything...
	int32 count = CountItems();
	RemoveItems(0, count, true);

	if (!projectPath.IsEmpty()) {
		Genio::Git::GitRepository* repo = nullptr;
		try {
			repo = new Genio::Git::GitRepository(projectPath);
			auto branches = repo->GetBranches();
			auto current_branch = repo->GetCurrentBranch();
			for(auto &branch : branches) {
				BMessage *message = new BMessage(fMessage->what);
				message->AddString("branch", branch);
				message->AddString("project_path", projectPath);
				auto item = new BMenuItem(branch, message);
				AddItem(item);
				if (branch == current_branch)
					item->SetMarked(true);
			}
		} catch(...) {
		}
		delete repo;
	}
	return count > 0;
}
