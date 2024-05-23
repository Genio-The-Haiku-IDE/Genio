/*
 * Copyright 2023, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 * Author: Nexus6 <nexus6@disroot.org>
 */
#pragma once

#include <String.h>

#include <vector>

struct Extension {
	BString Name;
	BString Description;
	bool ExtrasMenu;
	bool ContextMenu;
	uint32 Modifier;
	char Shortcut;
	// BString type;
	std::vector<BString> Context;
	std::vector<BString> Files;
};

class ExtensionManager {
public:
	ExtensionManager();

	std::vector<Extension> Extensions;

private:
	void _ScanExtensions(BString directory);
	BString _GetSystemDirectory();
	BString _GetUserDirectory();

};

