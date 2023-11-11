/*
 * Copyright 2017 A. Mosca 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GenioAPP_H
#define GenioAPP_H

#include <Application.h>
#include <Path.h>

class ConfigManager;
class GenioWindow;
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
		BPath					fConfigurationPath;
};

extern ConfigManager gCFG;

#endif //GenioAPP_H
