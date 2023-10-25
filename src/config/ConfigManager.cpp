/*
 * Copyright 2018, Genio
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConfigManager.h"
#include <Path.h>
#include <File.h>
#include <assert.h>
#include "Log.h"

ConfigManager gConfigManager;
ConfigManager& gCFG = gConfigManager;

ConfigManager::ConfigManager(){
	assert(fLocker.InitCheck() == B_OK);
}

auto ConfigManager::operator[](const char* key) -> ConfigManagerReturn {
	type_code type;
	if (storage.GetInfo(key, &type) != B_OK) {
		printf("No info for key [%s]\n", key);
		throw new std::exception();
	}
	return ConfigManagerReturn(storage, key, fLocker);
}

bool ConfigManager::Has(GMessage& msg, const char* key) {
	type_code type;
	return (msg.GetInfo(key, &type) == B_OK);
}

status_t	
ConfigManager::LoadFromFile(BPath path)
{
	GMessage fromFile;
	BFile file;
	status_t status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		status = fromFile.Unflatten(&file);
		if (status == B_OK) {
			//printf("configs from file:"); fromFile.PrintToStream();
			GMessage msg;
			int i = 0;
			while (configuration.FindMessage("config", i++, &msg) == B_OK) {
				if (fromFile.Has(msg["key"])) { //TODO check the type!! 
					(*this)[msg["key"]] = fromFile[msg["key"]];
					LogInfo("Configuration files loading value for key [%s]", (const char*)msg["key"]);
				} else {
					LogError("Configuration files does not contain key [%s]", (const char*)msg["key"]);
				}
			}
			//printf("configs after the file:"); storage.PrintToStream();
		}
	}
	return status;
}


status_t
ConfigManager::SaveToFile(BPath path)
{
	BFile file;
	status_t status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (status == B_OK) {
		status = storage.Flatten(&file);
	}
	return status;
}


void 
ConfigManager::ResetToDefault()
{
	GMessage msg;
	int i = 0;
	while (configuration.FindMessage("config", i++, &msg) == B_OK) {
		storage[msg["key"]] = msg["default_value"];
	}
}


void
ConfigManager::PrintAll()
{
	PrintValues();
	configuration.PrintToStream();
}


void
ConfigManager::PrintValues()
{
	storage.PrintToStream();
}
