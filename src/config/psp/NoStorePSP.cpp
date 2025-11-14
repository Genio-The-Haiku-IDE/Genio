/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "NoStorePSP.h"

NoStorePSP::NoStorePSP()
{
}

status_t NoStorePSP::Open(const BPath& destination, kPSPMode mode)
{
	return B_OK;
}

status_t NoStorePSP::Close()
{
	return B_OK;
}

status_t NoStorePSP::LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& parConfig)
{
	return B_OK;
}

status_t NoStorePSP::SaveKey(ConfigManager& manager, const char* key, GMessage& storage)
{
	return B_OK;
}
