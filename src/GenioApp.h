/*
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * Copyright 2023-2025, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <Application.h>
#include <Catalog.h>
#include <Path.h>
#include <StringList.h>


namespace GenioNames
{
	const BString kApplicationName(B_TRANSLATE_SYSTEM_NAME("Genio"));
	const BString kApplicationSignature("application/x-vnd.Genio");
	const BString kSettingsFileName("Genio.settings");
	const BString kSettingsFilesToReopen("files_to_reopen.settings");
	const BString kSettingsProjectsToReopen("workspace.settings");
	const BString kProjectSettingsFile(".genio");
}


class ConfigManager;
class ExtensionManager;
class GenioWindow;
class GenioApp : public BApplication {
public:
						GenioApp();
	virtual				~GenioApp();

		void			AboutRequested() override;
		void			ArgvReceived(int32 agrc, char** argv) override;
		void			MessageReceived(BMessage* message) override;
		bool			QuitRequested() override;
		void			ReadyToRun() override;
		void			RefsReceived(BMessage* message) override;

		// Scripting
		status_t		GetSupportedSuites(BMessage* data) override;
		BHandler*		ResolveSpecifier(BMessage* message, int32 index,
							BMessage* specifier, int32 what, const char* property) override;

		ExtensionManager*	GetExtensionManager() const { return fExtensionManager; }

private:
	int					_HandleArgs(int argc, char **argv);
	void				_HandleScripting(BMessage* data);
	static void			_PrepareConfig(ConfigManager& cfg);
	static BStringList	_SplitChangeLog(const char* changeLog);

	GenioWindow*		fGenioWindow;
	BPath				fConfigurationPath;
	ExtensionManager*	fExtensionManager;
};

extern ConfigManager gCFG;
