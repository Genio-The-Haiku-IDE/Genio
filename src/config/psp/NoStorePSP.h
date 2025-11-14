/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef NO_STORE_PSP_H
#define NO_STORE_PSP_H

#include "PermanentStorageProvider.h"

class NoStorePSP : public PermanentStorageProvider {
public:
	NoStorePSP();

	status_t Open(const BPath& destination, kPSPMode mode) override;
	status_t Close() override;
	status_t LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& parConfig) override;
	status_t SaveKey(ConfigManager& manager, const char* key, GMessage& storage) override;
};

#endif // NO_STORE_PSP_H
