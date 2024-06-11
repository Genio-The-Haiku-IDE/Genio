/*
 * Copyright 2023, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6@disroot.org>
 */

#include "ToolsMenu.h"

#include <Application.h>
#include <Catalog.h>
#include <Directory.h>
#include <MenuItem.h>
#include <MimeTypes.h>
#include <cstdio>

#include "GenioApp.h"
#include "GenioWindowMessages.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ToolsMenu"


ToolsMenu::ToolsMenu(const char* label, uint32 command, BHandler *target,
	std::function<ExtensionContext()> context_lambda)
	:
	BMenu(label),
	fCommand(command),
	fTarget(target),
	fContextLambda(context_lambda)
{

}


ToolsMenu::~ToolsMenu()
{
}


void
ToolsMenu::AttachedToWindow()
{
	_BuildMenu();
	SetTargetForItems(fTarget);
	BMenu::AttachedToWindow();
}


void
ToolsMenu::_BuildMenu()
{
	RemoveItems(0, CountItems(), true);

	auto app = reinterpret_cast<GenioApp *>(be_app);
	auto extManager = app->GetExtensionManager();
	auto context = fContextLambda();
	auto extensions = extManager->GetScriptExtensions(context);
	if (extensions.size() > 0) {
		for (auto &extension: extensions) {
			BMessage *extensionMessage = new BMessage(fCommand);
			extensionMessage->AddString("ShortDescription", extension.ShortDescription);
			extensionMessage->AddRef("refs", &extension.Ref);

			auto item = new BMenuItem(extension.ShortDescription, extensionMessage,
				extension.Shortcut, B_OPTION_KEY);

			item->SetEnabled(extension.Enabled);
			AddItem(item);
		}
	} else {
		SetEnabled(false);
	}
}
