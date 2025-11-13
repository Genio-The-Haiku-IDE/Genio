/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AttributePSP.h"
#include "ConfigManager.h"
#include "GMessage.h"
#include "Log.h"
#include <String.h>
#include <Application.h>

AttributePSP::AttributePSP()
{
}

status_t AttributePSP::Open(const BPath& attributeFilePath, kPSPMode mode)
{
	return fNodeAttr.SetTo(attributeFilePath.Path());
}

status_t AttributePSP::Close()
{
	return B_OK;
}

status_t AttributePSP::LoadKey(ConfigManager& manager, const char* key,
	GMessage& storage, GMessage& parameterConfig)
{
	const void* data = nullptr;
	ssize_t numBytes = 0;
	type_code type = parameterConfig.Type("default_value");
	if (parameterConfig.FindData("default_value", type, &data, &numBytes) == B_OK) {
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

status_t AttributePSP::SaveKey(ConfigManager& manager, const char* key, GMessage& storage)
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