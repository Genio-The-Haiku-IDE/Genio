/*
 * Copyright 2018, Genio
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ConfigManager.h"

#include <cassert>

#include <File.h>
#include <Path.h>

#include "Log.h"


ConfigManager::ConfigManager(const int32 messageWhat)
	:
	fLocker("ConfigManager lock"),
	fWhat(messageWhat)
{
	assert(fLocker.InitCheck() == B_OK);
}


auto ConfigManager::operator[](const char* key) -> ConfigManagerReturn
{
	return ConfigManagerReturn(key, *this);
}

bool
ConfigManager::_CheckKeyIsValid(const char* key) const
{
	type_code type;
	if (storage.GetInfo(key, &type) != B_OK) {
		BString detail("No config key: ");
		detail << key;
		debugger(detail.String());
		LogFatal(detail.String());
		throw new std::exception();
	}
	return true;
}

bool
ConfigManager::Has(GMessage& msg, const char* key) const
{
	type_code type;
	return (msg.GetInfo(key, &type) == B_OK);
}


status_t
ConfigManager::LoadFromFile(BPath messageFilePath, BPath attributeFilePath)
{
	GMessage fromFile;
	BFile file;
	BNode* nodeAttr = nullptr;
	status_t status = file.SetTo(messageFilePath.Path(), B_READ_ONLY);
	if (status == B_OK) {
		status = fromFile.Unflatten(&file);
		if (status == B_OK) {
			GMessage msg;
			int i = 0;
			while (configuration.FindMessage("config", i++, &msg) == B_OK) {
				const char* key = msg["key"];
				if ((bool)msg["as_attribute"] == false) {
					if (fromFile.Has(key) && _SameTypeAndFixedSize(&fromFile, key, &storage, key)) {
						(*this)[key] = fromFile[key];
						LogInfo("Configuration files loading value for key [%s]", (const char*)msg["key"]);
					} else {
						LogError("Configuration files does not contain the vaid key [%s]", (const char*)msg["key"]);
					}
				} else {
					if (attributeFilePath == BPath()) {
						LogError("Can't load a config file attribute. No path specified!");
						continue;
					}
					if (nodeAttr == nullptr) {
						nodeAttr = new BNode();
						status_t statusFile = nodeAttr->SetTo(attributeFilePath.Path());
						if (statusFile != B_OK) {
							LogErrorF("file error (%s) on loading attribute %s", strerror(statusFile), key);
							delete nodeAttr;
							nodeAttr = nullptr;
							continue;
						}
					}

					// save as attribute:
					BString attrName("genio:");
					attrName.Append(key);
					const void* data = nullptr;
					ssize_t numBytes = 0;
					type_code type = msg.Type("default_value");
					if (msg.FindData("default_value", type, &data, &numBytes) == B_OK) {
						void* buffer = malloc(numBytes);
						ssize_t readStatus = nodeAttr->ReadAttr(attrName.String(), type, 0, buffer, numBytes);
						if (readStatus == B_ENTRY_NOT_FOUND)
							LogInfo("Attribute not found! [%s] on [%s]", attrName.String(), attributeFilePath.Path());
						else if (readStatus <= 0) {
							LogError("Can't load config from attribute: [%s] on [%s] (%d)", attrName.String(), attributeFilePath.Path(), readStatus);
						} else {
							storage.RemoveName(key);
							if (storage.AddData(key, type, buffer, numBytes) == B_OK) {
								GMessage noticeMessage(fWhat);
								noticeMessage["key"]  	= key;
								noticeMessage["value"]  = storage[key];
								if (be_app != nullptr)
									be_app->SendNotices(fWhat, &noticeMessage);
							} else {
								LogError("Can't add config from attribute: [%s] on [%s] (%d)", attrName.String(), attributeFilePath.Path(), readStatus);
							}
						}
						free(buffer);
					}
				}
			}
		}
	}
	storage.PrintToStream();
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
ConfigManager::SaveToFile(BPath messageFilePath, BPath attributeFilePath)
{
	BFile file;
	BNode* fileAttr = nullptr;
	status_t status = file.SetTo(messageFilePath.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	if (status == B_OK) {
			GMessage outFile;
			GMessage msg;
			int i = 0;
			while (configuration.FindMessage("config", i++, &msg) == B_OK) {
				const char* key = msg["key"];

				if ((bool)msg["as_attribute"] == false) {
					outFile[key] = storage[key];
				} else {

					if (attributeFilePath == BPath()) {
						LogError("Can't save a config file attribute. No path specified!");
						continue;
					}
					if (fileAttr == nullptr) {
						fileAttr = new BNode();
						status_t statusFile = fileAttr->SetTo(attributeFilePath.Path());
						if (statusFile != B_OK) {
							LogErrorF("file error (%s) on saving attribute %s", strerror(statusFile), key);
							delete fileAttr;
							fileAttr = nullptr;
							continue;
						}
					}

					// save as attribute:
					BString attrName("genio:");
					attrName.Append(key);
					const void* data = nullptr;
					ssize_t numBytes = 0;
					type_code type = storage.Type(key);
					if (storage.FindData(key, type, &data, &numBytes) == B_OK) {
						if (fileAttr->WriteAttr(attrName.String(), type, 0, data, numBytes) <= 0) {
							LogError("Can't save config as attribute: %s\n", attrName.String());
						}
					}
				}
			}
			status = outFile.Flatten(&file);
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
