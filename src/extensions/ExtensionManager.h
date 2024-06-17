/*
 * Copyright 2024, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 
 */
#pragma once

#include <Entry.h>
#include <Message.h>
#include <vector>

#include <String.h>

class Editor;
class ProjectItem;

struct ExtensionInfo {
	bool Enabled;
	BString Name;
	BString Signature;
	BString ShortDescription;
	BString LongDescription;
	BString Version;
	entry_ref Ref;
	bool ShowInToolsMenu;
	bool ShowInContextMenu;
	uint32 Modifier;
	char Shortcut;
	BString Type;
	std::vector<BString> Scope;
	std::vector<BString> FileTypes;
};

enum ExtensionMenuKind {
	ExtensionToolsMenu,
	ExtensionContextMenu
};

struct ExtensionContext {
	ExtensionMenuKind menuKind;
	Editor* editor = nullptr;
	ProjectItem* projectItem = nullptr;
};


class ExtensionManager {
public:
	ExtensionManager();

	std::vector<ExtensionInfo>& GetExtensions();

private:
	void _ScanExtensions(BString directory);
	BString _GetSystemDirectory();
	BString _GetUserDirectory();

	std::vector<ExtensionInfo> fExtensions;
};

