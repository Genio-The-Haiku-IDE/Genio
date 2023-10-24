/*
 * Copyright 2018, Genio
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConfigManager.h"
#include <Path.h>
#include <File.h>
#include "Log.h"

ConfigManager gConfigManager;
ConfigManager& gCFG = gConfigManager;

status_t	
ConfigManager::LoadFromFile(BPath path) {
	GMessage fromFile;
	BFile file;
	status_t status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		status = fromFile.Unflatten(&file);
		if (status == B_OK) {
			GMessage msg;
			int i=0;
			while(configuration.FindMessage("config", i++, &msg) == B_OK) {
				storage[msg["key"]] = msg["default_value"];
				if (fromFile.Has(msg["key"])){ //TODO check the type!! 
					storage[msg["key"]] = fromFile["key"];
					LogInfo("Configuration files loading value for key [%s]", (const char*)msg["key"]);
				} else {
					LogInfo("Configuration files does not contain key [%s]", (const char*)msg["key"]);
				}
			}
		}
	}
	return status;
}