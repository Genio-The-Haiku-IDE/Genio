/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "BMessagePSP.h"
#include "ConfigManager.h"

BMessagePSP::BMessagePSP()
{
}

status_t BMessagePSP::Open(const BPath& dest, kPSPMode mode)
{
	uint32 fileMode = mode == PermanentStorageProvider::kPSPReadMode ? B_READ_ONLY : (B_WRITE_ONLY | B_CREATE_FILE);
	status_t status = fFile.SetTo(dest.Path(), fileMode);
	if (status == B_OK && fFile.IsReadable()) {
		// TODO: if file is not readable we would still return B_OK
		status = fFromFile.Unflatten(&fFile);
	}
	return status;
}

status_t BMessagePSP::Close()
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

status_t BMessagePSP::LoadKey(ConfigManager& manager, const char* key,
							GMessage& storage, GMessage& parameterConfig)
{
	if (fFromFile.Has(key) && _SameTypeAndFixedSize(&fFromFile, key, &storage, key)) {
		manager[key] = fFromFile[key];
		return B_OK;
	}
	return B_NAME_NOT_FOUND;
}

status_t BMessagePSP::SaveKey(ConfigManager& manager, const char* key, GMessage& storage)
{
	fFromFile[key] = storage[key];
	return B_OK;
}

bool BMessagePSP::_SameTypeAndFixedSize(
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
