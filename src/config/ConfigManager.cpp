/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ConfigManager.h"

#include <File.h>
#include <Path.h>

#include "Log.h"

class PermanentStorageProvider {
public:
	enum kPSPMode { kPSPReadMode, kPSPWriteMode };

						PermanentStorageProvider() {};
						virtual ~PermanentStorageProvider() {};

	virtual status_t	Open(const BPath& destination, kPSPMode mode) = 0;
	virtual status_t	Close() = 0;
	virtual status_t	LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& parConfig) = 0;
	virtual status_t	SaveKey(ConfigManager& manager, const char* key, GMessage& storage) = 0;

};

class BMessagePSP : public PermanentStorageProvider {
public:
		BMessagePSP()
		{
		}
		virtual status_t Open(const BPath& dest, kPSPMode mode)
		{
			uint32 fileMode = mode == PermanentStorageProvider::kPSPReadMode ? B_READ_ONLY : (B_WRITE_ONLY | B_CREATE_FILE);
			status_t status = fFile.SetTo(dest.Path(), fileMode);
			if (status == B_OK && fFile.IsReadable()) {
				// TODO: if file is not readable we would still return B_OK
				status = fFromFile.Unflatten(&fFile);
			}
			return status;
		}
		virtual status_t Close()
		{
			status_t status = fFile.InitCheck();
			if (status == B_OK && fFile.IsWritable()) {
				// TODO: if file is not writable we would still return B_OK
				status = fFromFile.Flatten(&fFile);
				if (status == B_OK) {
					status = fFile.Flush();
				}
				fFile.Unset();
				return status;
			}
			return status;
		}
		virtual status_t LoadKey(ConfigManager& manager, const char* key,
								GMessage& storage, GMessage& paramerConfig)
		{
			if (fFromFile.Has(key) && _SameTypeAndFixedSize(&fFromFile, key, &storage, key)) {
				manager[key] = fFromFile[key];
				return B_OK;
			}
			return B_NAME_NOT_FOUND;
		}
		virtual status_t SaveKey(ConfigManager& manager, const char* key, GMessage& storage)
		{
			fFromFile[key] = storage[key];
			return B_OK;
		}

	private:
		BFile fFile;
		GMessage fFromFile;

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
	AttributePSP()
	{
	}

	virtual status_t Open(const BPath& attributeFilePath, kPSPMode mode)
	{
		return fNodeAttr.SetTo(attributeFilePath.Path());
	}

	virtual status_t Close()
	{
		return B_OK;
	}

	virtual status_t LoadKey(ConfigManager& manager, const char* key,
		GMessage& storage, GMessage& paramerConfig)
	{
		const void* data = nullptr;
		ssize_t numBytes = 0;
		type_code type = paramerConfig.Type("default_value");
		if (paramerConfig.FindData("default_value", type, &data, &numBytes) == B_OK) {
			BString attrName("genio:");
			attrName.Append(key);
			void* buffer = ::malloc(numBytes);
			if (buffer == nullptr)
				return B_NO_MEMORY;
			ssize_t readStatus = fNodeAttr.ReadAttr(attrName.String(), type, 0, buffer, numBytes);
			if (readStatus <= 0) {
				::free(buffer);
				return B_NAME_NOT_FOUND;
			}
			storage.RemoveName(key);
			status_t st = storage.AddData(key, type, buffer, numBytes);
			if (st == B_OK) {
				GMessage noticeMessage(manager.UpdateMessageWhat());
				noticeMessage["key"] = key;
				noticeMessage["value"] = storage[key];
				if (be_app != nullptr)
					be_app->SendNotices(manager.UpdateMessageWhat(), &noticeMessage);
			}
			::free(buffer);
			return st;
		}
		return B_NAME_NOT_FOUND;
	}

	virtual status_t SaveKey(ConfigManager& manager, const char* key, GMessage& storage)
	{
		// save as attribute:
		const void* data = nullptr;
		ssize_t numBytes = 0;
		type_code type = storage.Type(key);
		if (storage.FindData(key, type, &data, &numBytes) == B_OK) {
			BString attrName("genio:");
			attrName.Append(key);
			if (fNodeAttr.WriteAttr(attrName.String(), type, 0, data, numBytes) <= 0) {
				return B_ERROR;
			}
		}
		return B_OK;
	}
private:
	BNode fNodeAttr;
};


class NoStorePSP : public PermanentStorageProvider {
public:
						NoStorePSP() {}

	virtual status_t	Open(const BPath& destination, kPSPMode mode) { return B_OK; }
	virtual status_t	Close() { return B_OK; }
	virtual status_t	LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& parConfig) { return B_OK; }
	virtual status_t	SaveKey(ConfigManager& manager, const char* key, GMessage& storage) { return B_OK; }
};


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
		case kStorageTypeBMessage:
			return new BMessagePSP();
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
			return B_ERROR;
		}
	}

	type_code typeFound;
	int32 countFound = 0;
	if (fConfiguration.GetInfo("config", &typeFound, &countFound) != B_OK) {
		LogError("LoadFromFile: no config configured!");
		return B_OK;
	}

	// TODO: Unify "context" with ResetToDefault one and rename it to "stage" (start/end) ?
	fNoticeMessage.RemoveData("context");
	fNoticeMessage.AddString("context", "load_from_file");

	status_t status = B_OK;
	GMessage msg;
	int32 i = 0;
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		const char* key = msg["key"];
		StorageType storageType = (StorageType)((int32)msg["storage_type"]);
		PermanentStorageProvider* provider = fPSPList[storageType];
		if (provider == nullptr) {
			LogErrorF("Invalid  PermanentStorageProvider (%d)", storageType);
			return B_ERROR;
		}

		if (countFound == i)
			fNoticeMessage.ReplaceString("context", "load_from_file_end");

		status = provider->LoadKey(*this, key, fStorage, msg);
		if (status == B_OK) {
			LogInfo("Config file: loaded value for key [%s] (StorageType %d)", key, storageType);
		} else {
			LogError("Config file: unable to get valid key [%s] (%s) (StorageType %d)",
				key, ::strerror(status), storageType);
		}
	}

	fNoticeMessage.RemoveData("context");

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

	fNoticeMessage.RemoveData("context");
	fNoticeMessage.AddString("context", "reset_to_defaults");
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		if (countFound == i)
			fNoticeMessage.ReplaceString("context", "reset_to_defaults_end");

		fStorage[msg["key"]] = msg["default_value"]; //to force the key creation
		(*this)[msg["key"]] = msg["default_value"]; //to force the update
	}

	fNoticeMessage.RemoveData("context");
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
