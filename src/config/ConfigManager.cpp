/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ConfigManager.h"

#include <File.h>
#include <Path.h>
#include <Entry.h>
#include <sstream>
#include <vector>

#include "Log.h"
#include "psp/PermanentStorageProvider.h"
#include "psp/YamlPSP.h"
#include "psp/BMessagePSP.h"
#include "psp/AttributePSP.h"
#include "psp/NoStorePSP.h"


// ConfigManager
ConfigManager::ConfigManager(const int32 messageWhat)
	:
	fLocker("ConfigManager lock")
{
	fNoticeMessage.what = messageWhat;
	if (fLocker.InitCheck() != B_OK)
		throw std::runtime_error("ConfigManager: Failed initializing locker!");
	for (int32 i = 0; i < kStorageTypeCountNb; i++)
		fPSPList[i] = nullptr;
}


ConfigManager::~ConfigManager()
{
	for (int32 i = 0; i< kStorageTypeCountNb; i++) {
		delete fPSPList[i];
		fPSPList[i] = nullptr;
	}
}


status_t
ConfigManager::FindConfigMessage(const char* name, int32 index,
									BMessage* message)
{
	BAutolock lock(fLocker);
	return fConfiguration.FindMessage(name, index, message);
}


PermanentStorageProvider*
ConfigManager::CreatePSPByType(StorageType type)
{
	switch (type) {
		case kStorageTypeYaml:
			return new YamlPSP();
		case kStorageTypeAttribute:
			return new AttributePSP();
		case kStorageTypeNoStore:
			return new NoStorePSP();
		default:
			LogErrorF("Invalid StorageType! %d", type);
			return nullptr;
	}
	return nullptr;
}


auto
ConfigManager::operator[](const char* key) -> ConfigManagerReturn
{
	return ConfigManagerReturn(key, *this);
}


bool
ConfigManager::HasKey(const char* key) const
{
	BAutolock lock(fLocker);
	type_code type;
	return fStorage.GetInfo(key, &type) == B_OK;
}


status_t
ConfigManager::LoadFromFile(std::array<BPath, kStorageTypeCountNb> paths)
{
	BAutolock lock(fLocker);
	for (int32 i = 0; i < kStorageTypeCountNb; i++) {
		if (fPSPList[i] != nullptr &&
			fPSPList[i]->Open(paths[i], PermanentStorageProvider::kPSPReadMode) != B_OK) {
			LogErrorF("Failed to open PermanentStorageProvider (%d)!", i);
			return B_ERROR;
		} else {
			LogInfoF("PermanentStorageProvider (%d) opened successfully.", i);
		}
	}

	type_code typeFound;
	int32 countFound = 0;
	if (fConfiguration.GetInfo("config", &typeFound, &countFound) != B_OK) {
		LogError("LoadFromFile: no config configured!");
		return B_OK;
	}

	// TODO: Unify "context" with ResetToDefault one and rename it to "stage" (start/end) ?
	fNoticeMessage.RemoveData(kContext);
	fNoticeMessage.AddString(kContext, "load_from_file");

	status_t status = B_OK;
	GMessage msg;
	int32 i = 0;
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		const char* key = msg["key"];
		StorageType storageType = (StorageType)((int32)msg["storage_type"]);
		LogDebug("Loading key [%s] from configuration setting (%d/%d) for provider %d", key,i,countFound, storageType);
		PermanentStorageProvider* provider = fPSPList[storageType];
		if (provider == nullptr) {
			LogErrorF("Invalid  PermanentStorageProvider (%d)", storageType);
			return B_ERROR;
		}

		if (countFound == i)
			fNoticeMessage.ReplaceString(kContext, "load_from_file_end");

		status = provider->LoadKey(*this, key, fStorage, msg);
		if (status == B_OK) {
			LogInfo("Config file: loaded value for key [%s] (StorageType %d)", key, storageType);
		} else {
			LogError("Config file: unable to get valid key [%s] (%s) (StorageType %d)",
				key, ::strerror(status), storageType);
		}
	}

	fNoticeMessage.RemoveData(kContext);

	for (int32 i = 0; i < kStorageTypeCountNb; i++) {
		if (fPSPList[i] != nullptr)
			fPSPList[i]->Close();
	}
	return status;
}


status_t
ConfigManager::SaveToFile(std::array<BPath, kStorageTypeCountNb> paths)
{
	BAutolock lock(fLocker);
	for (int32 i = 0; i < kStorageTypeCountNb; i++) {
		if (fPSPList[i] != nullptr &&
			fPSPList[i]->Open(paths[i], PermanentStorageProvider::kPSPWriteMode) != B_OK) {
			return B_ERROR;
		}
	}

	status_t status = B_OK;
	GMessage msg;
	int32 i = 0;
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		const char* key = msg["key"];
		StorageType storageType = (StorageType)((int32)msg["storage_type"]);
		PermanentStorageProvider* provider = fPSPList[storageType];
		if (provider == nullptr) {
			LogErrorF("Invalid PermanentStorageProvider (%d)", storageType);
			return B_ERROR;
		}
		status = provider->SaveKey(*this, key, fStorage);
		if (status == B_OK) {
			LogInfo("Config file: saved value for key [%s] (StorageType %d)", key, storageType);
		} else {
			LogError("Config file: unable to store valid key [%s] (%s) (StorageType %d)",
				key, ::strerror(status), storageType);
		}
	}
	for (int32 i = 0; i < kStorageTypeCountNb; i++) {
		if (fPSPList[i] != nullptr)
			fPSPList[i]->Close();
	}
	return status;
}


void
ConfigManager::ResetToDefaults()
{
	BAutolock lock(fLocker);
	// Will also send notifications for every setting change
	GMessage msg;
	int32 i = 0;
	type_code typeFound;
	int32 countFound = 0;
	if (fConfiguration.GetInfo("config", &typeFound, &countFound) != B_OK) {
		LogError("ResetToDefaults: no config configured!");
		return;
	}

	fNoticeMessage.RemoveData(kContext);
	fNoticeMessage.AddString(kContext, "reset_to_defaults");
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		if (countFound == i)
			fNoticeMessage.ReplaceString(kContext, "reset_to_defaults_end");

		fStorage[msg["key"]] = msg["default_value"]; //to force the key creation
		(*this)[msg["key"]] = msg["default_value"]; //to force the update
	}

	fNoticeMessage.RemoveData(kContext);
}


bool
ConfigManager::HasAllDefaultValues()
{
	BAutolock lock(fLocker);
	GMessage msg;
	int32 i = 0;
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		if (fStorage[msg["key"]] != msg["default_value"]) {
			const char* key = msg["key"];
			LogDebug("Differs for key %s\n", key);
			return false;
		}
	}
	return true;
}


void
ConfigManager::PrintAll() const
{
	BAutolock lock(fLocker);
	PrintValues();
	fConfiguration.PrintToStream();
}


void
ConfigManager::PrintValues() const
{
	BAutolock lock(fLocker);
	fStorage.PrintToStream();
}


bool
ConfigManager::_CheckKeyIsValid(const char* key) const
{
	if (!fLocker.IsLocked())
		throw std::runtime_error("_CheckKeyIsValid(): Locker is not locked!");

	type_code type;
	if (fStorage.GetInfo(key, &type) != B_OK) {
		BString detail("No config key: ");
		detail << key;
		debugger(detail.String());
		LogFatal(detail.String());
		throw std::runtime_error(detail.String());
	}
	return true;
}
