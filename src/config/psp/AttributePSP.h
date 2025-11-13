/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef ATTRIBUTE_PSP_H
#define ATTRIBUTE_PSP_H

#include "PermanentStorageProvider.h"
#include <Node.h>

class AttributePSP : public PermanentStorageProvider {
public:
	AttributePSP();
	
	virtual status_t Open(const BPath& attributeFilePath, kPSPMode mode) override;
	virtual status_t Close() override;
	virtual status_t LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& parameterConfig) override;
	virtual status_t SaveKey(ConfigManager& manager, const char* key, GMessage& storage) override;

private:
	BNode fNodeAttr;
};

#endif // ATTRIBUTE_PSP_H
