/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef BMESSAGE_PSP_H
#define BMESSAGE_PSP_H

#include "PermanentStorageProvider.h"
#include <File.h>
#include "GMessage.h"

class BMessagePSP : public PermanentStorageProvider {
public:
	BMessagePSP();

	status_t Open(const BPath& dest, kPSPMode mode) override;
	status_t Close() override;
	status_t LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& parameterConfig) override;
	status_t SaveKey(ConfigManager& manager, const char* key, GMessage& storage) override;

private:
	BFile fFile;
	GMessage fFromFile;

	bool _SameTypeAndFixedSize(BMessage* msgL, const char* keyL, BMessage* msgR, const char* keyR) const;
};

#endif // BMESSAGE_PSP_H
