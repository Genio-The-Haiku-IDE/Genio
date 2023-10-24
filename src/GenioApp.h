/*
 * Copyright 2017 A. Mosca <amoscaster@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GenioAPP_H
#define GenioAPP_H

#include <Application.h>

#include "GenioWindow.h"
#include "TPreferences.h"

class ConfigManager;
class GenioApp : public BApplication {
public:
								GenioApp();
	virtual						~GenioApp();

	virtual	void				AboutRequested();
	virtual	void				ArgvReceived(int32 agrc, char** argv);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
	virtual	void				ReadyToRun();
	virtual	void				RefsReceived(BMessage* message);
	void						PrepareConfig(ConfigManager& cfg);

private:
			void				_CheckSettingsVersion();
private:
		GenioWindow*			fGenioWindow;
		TPreferences*			fUISettingsFile;
};


// TODO: Use a static method ?
extern ConfigManager& gCFG;


#endif //GenioAPP_H
