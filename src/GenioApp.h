/*
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * Copyright 2023, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <Application.h>
#include <Path.h>
#include <StringList.h>

class ConfigManager;
class GenioWindow;
class GenioApp : public BApplication {
public:
							GenioApp();
	virtual					~GenioApp();

	virtual	void			AboutRequested();
	virtual	void			ArgvReceived(int32 agrc, char** argv);
	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	virtual	void			ReadyToRun();
	virtual	void			RefsReceived(BMessage* message);

private:
	static BStringList		_SplitChangeLog(const char* changeLog);
	static void				_SetSessionLogLevel(char level);
	int						_HandleArgs(int argc, char **argv);
	static void				_PrepareConfig(ConfigManager& cfg);

	GenioWindow*			fGenioWindow;
	BPath					fConfigurationPath;
};

extern ConfigManager gCFG;
