/*
 * Copyright 2018-2024, Genio
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConfigManager.h"

#include <cassert>

#include <File.h>
#include <Path.h>

#include "Log.h"

class PermanentStorageProvider {
public:
	enum kPSPMode { kPSPReadMode, kPSPWriteMode };

						PermanentStorageProvider(BPath destination, kPSPMode mode) {};

	virtual status_t	InitCheck() = 0;
	virtual status_t	LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& parConfig) = 0;
	virtual status_t	SaveKey(ConfigManager& manager, const char* key, GMessage& storage) = 0;

};

class BMessagePSP : public PermanentStorageProvider {
public:
		BMessagePSP(BPath dest, kPSPMode mode): PermanentStorageProvider(dest, mode){
			status = file.SetTo(dest.Path(), mode == PermanentStorageProvider::kPSPReadMode ? B_READ_ONLY : (B_WRITE_ONLY | B_CREATE_FILE));
			if (status == B_OK && file.IsReadable()) {
				status = fromFile.Unflatten(&file);
			}
		}
		~BMessagePSP() {
			if (status == B_OK && file.IsWritable()) {
				fromFile.Flatten(&file);
				file.Flush();
			}
		}

		status_t	InitCheck() {
			return status;
		}

		status_t	LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& paramerConfig) {
			if (fromFile.Has(key) && _SameTypeAndFixedSize(&fromFile, key, &storage, key)) {
				manager[key] = fromFile[key];
				return B_OK;
			}
			return B_NAME_NOT_FOUND;
		}
		status_t	SaveKey(ConfigManager& manager, const char* key, GMessage& storage) {
				fromFile[key] = storage[key];
				return B_OK;
		}

	private:
		BFile file;
		GMessage fromFile;
		status_t status = B_ERROR;

		bool _SameTypeAndFixedSize(
			BMessage* msgL, const char* keyL, BMessage* msgR, const char* keyR) const
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
};

class AttributePSP : public PermanentStorageProvider {
public:
		AttributePSP(BPath attributeFilePath, kPSPMode mode): PermanentStorageProvider(attributeFilePath, mode){
			status = nodeAttr.SetTo(attributeFilePath.Path());
		}

	status_t	InitCheck() { return status; }
	status_t	LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& paramerConfig)
	{
		BString attrName("genio:");
		attrName.Append(key);
		const void* data = nullptr;
		ssize_t numBytes = 0;
		type_code type = paramerConfig.Type("default_value");
		if (paramerConfig.FindData("default_value", type, &data, &numBytes) == B_OK) {
			void* buffer = malloc(numBytes);
			ssize_t readStatus = nodeAttr.ReadAttr(attrName.String(), type, 0, buffer, numBytes);
			if (readStatus <=0)
				return B_NAME_NOT_FOUND;

			storage.RemoveName(key);
			status_t st = storage.AddData(key, type, buffer, numBytes);
			if (st == B_OK) {
				GMessage noticeMessage(manager.UpdateMessageWhat());
				noticeMessage["key"] = key;
				noticeMessage["value"] = storage[key];
				if (be_app != nullptr)
					be_app->SendNotices(manager.UpdateMessageWhat(), &noticeMessage);
			}
			free(buffer);
			return st;
		}
		return B_NAME_NOT_FOUND;
	}
	status_t	SaveKey(ConfigManager& manager, const char* key, GMessage& storage) {
			// save as attribute:
			BString attrName("genio:");
			attrName.Append(key);
			const void* data = nullptr;
			ssize_t numBytes = 0;
			type_code type = storage.Type(key);
			if (storage.FindData(key, type, &data, &numBytes) == B_OK) {
				if (nodeAttr.WriteAttr(attrName.String(), type, 0, data, numBytes) <= 0) {
					return B_ERROR;
				}
			}
			return B_OK;
	}
private:
	BNode	nodeAttr;
	status_t status = B_ERROR;
};


ConfigManager::ConfigManager(const int32 messageWhat)
	:
	fLocker("ConfigManager lock"),
	fWhat(messageWhat)
{
	assert(fLocker.InitCheck() == B_OK);
}


auto
ConfigManager::operator[](const char* key) -> ConfigManagerReturn
{
	return ConfigManagerReturn(key, *this);
}


bool
ConfigManager::Has(GMessage& msg, const char* key) const
{
	type_code type;
	return msg.GetInfo(key, &type) == B_OK;
}


status_t
ConfigManager::LoadFromFile(BPath messageFilePath, BPath attributeFilePath)
{
	status_t status = B_OK;
	BMessagePSP storageM(messageFilePath, PermanentStorageProvider::kPSPReadMode);
	AttributePSP storageA(attributeFilePath, PermanentStorageProvider::kPSPReadMode);
	GMessage msg;
	int i = 0;
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		const char* key = msg["key"];
		StorageType storageType = (StorageType)((int32)msg["storage_type"]);
		status_t status = B_ERROR;
		if (storageType == kStorageTypeBMessage && storageM.InitCheck() == B_OK) {
			status = storageM.LoadKey(*this, key, fStorage, msg);
		} else if (storageType == kStorageTypeAttribute && storageA.InitCheck() == B_OK) {
			status = storageA.LoadKey(*this, key, fStorage, msg);
		}
		if (status == B_OK) {
			LogInfo("Config file: loaded value for key [%s] (StorageType %d)", key, storageType);
		} else {
			LogError("Config file: unable to get valid key [%s] (%s) (StorageType %d)", key, strerror(status), storageType);
		}
	}
	return status;
}

status_t
ConfigManager::SaveToFile(BPath messageFilePath, BPath attributeFilePath)
{
	status_t status = B_OK;
	BMessagePSP storageM(messageFilePath, PermanentStorageProvider::kPSPWriteMode);
	AttributePSP storageA(attributeFilePath, PermanentStorageProvider::kPSPWriteMode);
	GMessage msg;
	int i = 0;
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		const char* key = msg["key"];
		status_t status = B_ERROR;
		StorageType storageType = (StorageType)((int32)msg["storage_type"]);
		if (storageType == kStorageTypeBMessage && storageM.InitCheck() == B_OK) {
			status = storageM.SaveKey(*this, key, fStorage);
		} else if (storageType == kStorageTypeAttribute && storageA.InitCheck() == B_OK) {
			status = storageA.SaveKey(*this, key, fStorage);
		}
		if (status == B_OK) {
			LogInfo("Config file: saved value for key [%s] (StorageType %d)", key, storageType);
		} else {
			LogError("Config file: unable to store valid key [%s] (%s) (StorageType %d)", key, strerror(status), storageType);
		}
	}
	return status;
}


void
ConfigManager::ResetToDefaults()
{
	// Will also send notifications for every setting change
	GMessage msg;
	int i = 0;
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		fStorage[msg["key"]] = msg["default_value"]; //to force the key creation
		(*this)[msg["key"]] = msg["default_value"]; //to force the update
	}
}


bool
ConfigManager::HasAllDefaultValues()
{
	GMessage msg;
	int i = 0;
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		if (fStorage[msg["key"]] != msg["default_value"]) {
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
	fConfiguration.PrintToStream();
}


void
ConfigManager::PrintValues() const
{
	fStorage.PrintToStream();
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


bool
ConfigManager::_CheckKeyIsValid(const char* key) const
{
	type_code type;
	if (fStorage.GetInfo(key, &type) != B_OK) {
		BString detail("No config key: ");
		detail << key;
		debugger(detail.String());
		LogFatal(detail.String());
		throw new std::exception();
	}
	return true;
}
