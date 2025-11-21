/*
 * Copyright The Genio Contributors
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <Entry.h>
#include <Message.h>
#include <vector>

#include <String.h>

class Editor;
class ProjectItem;

struct ExtensionInfo {
	ExtensionInfo();

	BString Name;
	BString Signature;
	BString ShortDescription;
	BString LongDescription;
	BString Version;
	BString Type;

	entry_ref Ref;

	uint32 Modifier;
	char Shortcut;

	bool ShowInToolsMenu;
	bool ShowInContextMenu;

	bool Enabled;

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

