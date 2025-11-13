/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef YAML_PSP_H
#define YAML_PSP_H

#include "PermanentStorageProvider.h"
#include <File.h>
#include <yaml-cpp/yaml.h>

class BRect;
class BMessagePSP;

class YamlPSP : public PermanentStorageProvider {
public:
	YamlPSP();
	
	virtual status_t Open(const BPath& _dest, kPSPMode mode) override;
	virtual status_t Close() override;
	virtual status_t LoadKey(ConfigManager& manager, const char* key, GMessage& storage, GMessage& parameterConfig) override;
	virtual status_t SaveKey(ConfigManager& manager, const char* key, GMessage& storage) override;

private:
	YAML::Node yaml;
	BFile fFile;
    
    BMessagePSP* fBMsgPSP; //pointer to BMessagePSP for legacy format handling. To be removed in future versions

	status_t _LoadSingleValue(const YAML::Node& node, const char* key, GMessage& storage, type_code expectedType);
	bool _ParseRectFromString(const std::string& rectStr, BRect& rect);
	status_t _LoadMessage(const YAML::Node& node, GMessage& message);
	status_t _LoadMessageValue(const char* key, const YAML::Node& node, GMessage& message);
	status_t _SaveKey(YAML::Node& yaml, const char* key, GMessage& storage, int32 keyIndex);
};

#endif // YAML_PSP_H
