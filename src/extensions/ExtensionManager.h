/*
 * Copyright 2024, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6@disroot.org>
 */
#pragma once

#include <Entry.h>
#include <vector>

#include <String.h>

class Editor;
class ProjectFolder;

struct ExtensionInfo {
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


struct ExtensionContext {
	Editor* editor = nullptr;
	ProjectFolder* project = nullptr;
};


class ExtensionManager {
public:
	ExtensionManager();

	std::vector<ExtensionInfo>& GetExtensions(ExtensionContext& context) { return fExtensions; };

private:
	void _ScanExtensions(BString directory);
	BString _GetSystemDirectory();
	BString _GetUserDirectory();

	std::vector<ExtensionInfo> fExtensions;
};

