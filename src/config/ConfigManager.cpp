/*
 * Copyright 2018, Genio
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConfigManager.h"

#include <cassert>

#include <File.h>
#include <Path.h>

#include "Log.h"

ConfigManager gConfigManager;
ConfigManager& gCFG = gConfigManager;

ConfigManager::ConfigManager()
{
	assert(fLocker.InitCheck() == B_OK);
}


auto ConfigManager::operator[](const char* key) -> ConfigManagerReturn
{
	type_code type;
	if (storage.GetInfo(key, &type) != B_OK) {
		BString detail("No config key: ");
		detail << key;
		debugger(detail.String());
		LogFatal(detail.String());
		throw new std::exception();
	}
	return ConfigManagerReturn(storage, key, fLocker);
}


bool
ConfigManager::Has(GMessage& msg, const char* key) const
{
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
			GMessage msg;
			int i = 0;
			while (configuration.FindMessage("config", i++, &msg) == B_OK) {
				const char* key = msg["key"];
				if (fromFile.Has(key) && _SameTypeAndFixedSize(&fromFile, key, &storage, key)) {
					(*this)[msg["key"]] = fromFile[msg["key"]];
					LogInfo("Configuration files loading value for key [%s]", (const char*)msg["key"]);
				} else {
					LogError("Configuration files does not contain the vaid key [%s]", (const char*)msg["key"]);
				}
			}
		}
	}
	return status;
}


bool
ConfigManager::_SameTypeAndFixedSize(BMessage* msgL, const char* keyL,
									  BMessage* msgR, const char* keyR) const
{
	type_code typeL = 0;
	bool fixedSizeL = false;
	if (msgL->GetInfo(keyL, &typeL, &fixedSizeL) == B_OK) {
		type_code typeR = 0;
		bool fixedSizeR = false;
		if (msgR->GetInfo(keyR, &typeR, &fixedSizeR) == B_OK) {
			return (typeL == typeR && fixedSizeL == fixedSizeR);
		}
	}
	return false;
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
ConfigManager::ResetToDefaults()
{
	// Will also send notifications for every setting change
	GMessage msg;
	int i = 0;
	while (configuration.FindMessage("config", i++, &msg) == B_OK) {
		storage[msg["key"]] = msg["default_value"]; //to force the key creation
		(*this)[msg["key"]] = msg["default_value"]; //to force the update
	}
}


bool
ConfigManager::HasAllDefaultValues()
{
	GMessage msg;
	int i = 0;
	while (configuration.FindMessage("config", i++, &msg) == B_OK) {
		if (storage[msg["key"]] != msg["default_value"]) {
			LogDebug("Differs for key %s\n", (const char*)msg["key"]);
			return false;
		}
	}
	return true;
}


void
ConfigManager::PrintAll() const
{
	PrintValues();
	configuration.PrintToStream();
}


void
ConfigManager::PrintValues() const
{
	storage.PrintToStream();
}
