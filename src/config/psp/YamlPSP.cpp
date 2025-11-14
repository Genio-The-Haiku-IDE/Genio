/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "YamlPSP.h"
#include "ConfigManager.h"
#include "GMessage.h"
#include "Log.h"
#include <Path.h>
#include <Entry.h>
#include <String.h>
#include <Rect.h>
#include <sstream>
#include <vector>
#include "BMessagePSP.h"

#define _yaml_case(TYPE, pod, Name) \
			case TYPE: \
			{ \
				pod value; \
				status_t status = storage.Find ## Name (key, keyIndex, &value); \
				if ( status == B_OK) { \
					yaml[key] = value; \
				} \
				return status; \
			}

#define _yaml_case_string(TYPE, Name) \
			case TYPE: \
			{ \
				const char* value; \
				status_t status = storage.Find ## Name (key, keyIndex, &value); \
				if ( status == B_OK) { \
					BString safeValue(value); \
					yaml[key] = safeValue.String(); \
				} \
				return status; \
			}

#define _yaml_load(TYPE, pod) \
			case TYPE: \
			{ 	storage[key] = node.as<pod>(); return B_OK; }

YamlPSP::YamlPSP():fBMsgPSP(nullptr)
{
}

status_t YamlPSP::Open(const BPath& _dest, kPSPMode mode)
{
	//workaround.
	BPath dest;
	dest.SetTo(BString(_dest.Path()).Append(".yaml").String());

	uint32 fileMode = mode == PermanentStorageProvider::kPSPReadMode ? B_READ_ONLY : (B_WRITE_ONLY | B_CREATE_FILE);
	
	// Always try to load existing content first, even in write mode
	// to preserve existing data when updating configurations
	BEntry entry(dest.Path());
	if (entry.Exists()) {
		try {
			yaml = YAML::LoadFile(dest.Path());
			LogInfo("YAML file loaded successfully: %s", dest.Path());
		} catch(const YAML::ParserException& e) {
			LogError("YAML Parser Error in file %s at line %zu, column %zu: %s",
				dest.Path(), e.mark.line + 1, e.mark.column + 1, e.msg.c_str());
			yaml.reset(); // Reset to empty on parse error
			if (mode == kPSPReadMode) return B_ERROR;
		} catch(const YAML::BadFile& e) {
			LogError("YAML Bad File Error: Cannot open file %s: %s",
				dest.Path(), e.msg.c_str());
			yaml.reset(); // Reset to empty on file error
			if (mode == kPSPReadMode) return B_ERROR;
		} catch(const YAML::Exception& e) {
			LogError("YAML General Error in file %s: %s",
				dest.Path(), e.msg.c_str());
			yaml.reset(); // Reset to empty on YAML error
			if (mode == kPSPReadMode) return B_ERROR;
		} catch(const std::exception& e) {
			LogError("Standard Exception while loading YAML file %s: %s",
				dest.Path(), e.what());
			yaml.reset(); // Reset to empty on standard exception
			if (mode == kPSPReadMode) return B_ERROR;
		} catch(...) {
			LogError("Unknown exception while loading YAML file %s", dest.Path());
			yaml.reset(); // Reset to empty on unknown exception
			if (mode == kPSPReadMode) return B_ERROR;
		}
	} else {
		// .yaml file doesn't exist.
        //let's try the BMessage format (backward compatibility, to be removed!)
        if (mode == PermanentStorageProvider::kPSPReadMode &&
            BEntry(_dest.Path()).Exists()) {
                fBMsgPSP = new BMessagePSP();
                status_t status = fBMsgPSP->Open(_dest, mode);
                if (status == B_OK) {
                    LogInfo("Loaded configuration from legacy BMessage file %s", _dest.Path());
                    return B_OK;
                } else {
                    LogError("Failed to load legacy BMessage file %s: %s | Reverting to new YAML format.", _dest.Path(), strerror(status));
                }

        }
		yaml.reset();
	}

	// Now open the file for the requested operation
	status_t status = fFile.SetTo(dest.Path(), fileMode);
	if (status != B_OK) {
		LogError("Failed to open file %s with mode %s: %s",
			dest.Path(),
			(mode == kPSPReadMode) ? "READ" : "WRITE",
			::strerror(status));
	}
	return status;
}

status_t YamlPSP::Close()
{
    if (fBMsgPSP) {
        fBMsgPSP->Close();
        delete fBMsgPSP;
        fBMsgPSP = nullptr;
    }
	status_t status = fFile.InitCheck();
	if (status == B_OK && fFile.IsWritable()) {
		try {
			YAML::Emitter out;
			out << yaml;
			BString bout(out.c_str());

			// Ensure we're at the beginning of the file for complete rewrite
			fFile.Seek(0, SEEK_SET);
			
			ssize_t written = fFile.Write(bout.String(), bout.Length());
			if (written < 0) {
				LogError("Failed to write YAML data: %s", strerror(written));
				return (status_t)written;
			}
			
			// Truncate file to the exact size we wrote (in case file was larger before)
			fFile.SetSize(written);

			status = fFile.Flush();
			fFile.Unset();
			return status;
		} catch (const YAML::Exception& e) {
			LogError("YAML emission error: %s", e.what());
			fFile.Unset();
			return B_ERROR;
		} catch (...) {
			LogError("Unknown exception during YAML emission");
			fFile.Unset();
			return B_ERROR;
		}
	}
	fFile.Unset(); // Always unset the file
	return status;
}

status_t YamlPSP::LoadKey(ConfigManager& manager, const char* key,
						GMessage& storage, GMessage& parameterConfig)
{
    if (fBMsgPSP) {
        return fBMsgPSP->LoadKey(manager, key, storage, parameterConfig);
    }

	const YAML::Node fromFile = yaml[key];
	if (!fromFile) {
		LogError("LoadKey: Name not found for key [%s]", key);
		return B_NAME_NOT_FOUND;
	}

	LogDebug("LoadKey: Loading key [%s]", key);

	// Get the expected type from the parameter config
	type_code expectedType = parameterConfig.Type("default_value");
	if (expectedType == 0) {
		LogError("LoadKey: No default_value type found for key [%s]", key);
		return B_ERROR;
	}

	// Remove any existing data for this key
	storage.RemoveName(key);

	status_t status =  _LoadSingleValue(fromFile, key, storage, expectedType);
	LogInfo("Done with loading key %s (%s)", key, strerror(status));
	return status;
}

status_t YamlPSP::SaveKey(ConfigManager& manager, const char* key, GMessage& storage)
{
	// Check if this key has multiple values
	int32 count = 0;
	type_code type;
	if (storage.GetInfo(key, &type, &count) == B_OK && count > 1) {
		// Multiple values - create a YAML sequence
		YAML::Node sequenceNode(YAML::NodeType::Sequence);
		for (int32 i = 0; i < count; i++) {
			YAML::Node tempNode;
			if (_SaveKey(tempNode, key, storage, i) == B_OK) {
				// Extract the actual value from tempNode
				if (tempNode[key]) {
					sequenceNode.push_back(tempNode[key]);
				}
			}
		}
		yaml[key] = sequenceNode;
		return B_OK;
	} else {
		// Single value or no values
		return _SaveKey(yaml, key, storage, 0);
	}
}

status_t YamlPSP::_LoadSingleValue(const YAML::Node& node, const char* key, GMessage& storage, type_code expectedType)
{
	try {
		// Handle null values first for all types
		if (node.IsNull()) {
			switch (expectedType) {
				case B_STRING_TYPE:
					storage[key] = "";
					return B_OK;
				case B_MESSAGE_TYPE:
					storage[key] = GMessage();
					return B_OK;
				case B_BOOL_TYPE:
					storage[key] = false;
					return B_OK;
				case B_INT32_TYPE:
					storage[key] = (int32)0;
					return B_OK;
				case B_UINT32_TYPE:
					storage[key] = (uint32)0;
					return B_OK;
				case B_INT64_TYPE:
					storage[key] = (int64)0;
					return B_OK;
				case B_UINT64_TYPE:
					storage[key] = (uint64)0;
					return B_OK;
				case B_INT16_TYPE:
					storage[key] = (int16)0;
					return B_OK;
				case B_UINT16_TYPE:
					storage[key] = (uint16)0;
					return B_OK;
				case B_INT8_TYPE:
					storage[key] = (int8)0;
					return B_OK;
				case B_UINT8_TYPE:
					storage[key] = (uint8)0;
					return B_OK;
				case B_FLOAT_TYPE:
					storage[key] = (float)0.0;
					return B_OK;
				case B_DOUBLE_TYPE:
					storage[key] = (double)0.0;
					return B_OK;
				case B_RECT_TYPE:
					storage[key] = BRect(0,0,0,0);
					return B_OK;
				default:
					LogError("_LoadSingleValue: Null value not supported for type code: %ld", expectedType);
					return B_ERROR;
			}
		}

		switch (expectedType) {

			case B_STRING_TYPE:
			{
				std::string value = node.as<std::string>();
				storage[key] = value.c_str();
				return B_OK;
			}
			_yaml_load(B_BOOL_TYPE, bool)
			_yaml_load(B_INT32_TYPE, int32)
			_yaml_load(B_UINT32_TYPE, uint32)

			_yaml_load(B_INT64_TYPE, int64)
			_yaml_load(B_UINT64_TYPE, uint64)

			_yaml_load(B_INT16_TYPE, int16)
			_yaml_load(B_UINT16_TYPE, uint16)

			_yaml_load(B_INT8_TYPE, int8)
			_yaml_load(B_UINT8_TYPE, uint8)

			_yaml_load(B_FLOAT_TYPE, float)
			_yaml_load(B_DOUBLE_TYPE, double)

			case B_RECT_TYPE:
			{
				std::string rectStr = node.as<std::string>();
				BRect rect(0,0,0,0);
				if (_ParseRectFromString(rectStr, rect)) {
					storage[key] = rect;
					return B_OK;
				}
				LogError("_LoadSingleValue: Failed to parse rectangle string: %s", rectStr.c_str());
				return B_ERROR;
			}
			case B_MESSAGE_TYPE:
			{
				if (node.IsMap()) {
					GMessage subMessage;
					LogInfo("Loading BMessage for key %s\n", key);
					status_t status = _LoadMessage(node, subMessage);
					if (status == B_OK) {
						storage[key] = subMessage;
						LogInfo("Successfully loaded BMessage for key %s with %d fields", key, subMessage.CountNames(B_ANY_TYPE));
						return B_OK;
					}
					return status;
				}
				LogError("_LoadSingleValue: Expected map for B_MESSAGE_TYPE, got null:%d map:%d", node.IsNull(), node.IsMap());
				return B_ERROR;
			}
			case B_REF_TYPE:
			{
				std::string pathStr = node.as<std::string>();
				
				// Check if it's a ref:// prefixed string
				if (pathStr.substr(0, 6) == "ref://") {
					// Remove the ref:// prefix
					std::string actualPath = pathStr.substr(6);
					entry_ref ref;
					if (get_ref_for_path(actualPath.c_str(), &ref) == B_OK) {
						storage[key] = ref;
						return B_OK;
					}
					LogError("_LoadSingleValue: Failed to create entry_ref for path: %s", actualPath.c_str());
					return B_ERROR;
				} else {
					// Legacy format without prefix - try to handle it anyway
					entry_ref ref;
					if (get_ref_for_path(pathStr.c_str(), &ref) == B_OK) {
						storage[key] = ref;
						return B_OK;
					}
					LogError("_LoadSingleValue: Failed to create entry_ref for legacy path: %s", pathStr.c_str());
					return B_ERROR;
				}
			}
			default:
				LogError("_LoadSingleValue: Unsupported type code: %ld", expectedType);
				return B_ERROR;
		}
	} catch (const YAML::Exception& e) {
		LogError("_LoadSingleValue: YAML exception: %s", e.what());
		return B_ERROR;
	} catch (...) {
		LogError("_LoadSingleValue: Unknown exception occurred");
		return B_ERROR;
	}
}

bool YamlPSP::_ParseRectFromString(const std::string& rectStr, BRect& rect)
{
	// Expected format: "[left, top, right, bottom]"
	if (rectStr.length() < 7 || rectStr[0] != '[' || rectStr[rectStr.length()-1] != ']') {
		return false;
	}

	std::string content = rectStr.substr(1, rectStr.length() - 2); // Remove [ and ]
	std::vector<std::string> parts;
	std::stringstream ss(content);
	std::string item;

	while (std::getline(ss, item, ',')) {
		// Trim whitespace
		size_t start = item.find_first_not_of(" \t");
		size_t end = item.find_last_not_of(" \t");
		if (start != std::string::npos && end != std::string::npos) {
			parts.push_back(item.substr(start, end - start + 1));
		}
	}

	if (parts.size() != 4) {
		return false;
	}

	try {
		rect.left = std::stof(parts[0]);
		rect.top = std::stof(parts[1]);
		rect.right = std::stof(parts[2]);
		rect.bottom = std::stof(parts[3]);
		return true;
	} catch (...) {
		return false;
	}
}

status_t YamlPSP::_LoadMessage(const YAML::Node& node, GMessage& message)
{
	if (!node.IsMap()) {
		return B_ERROR;
	}

	for (auto it = node.begin(); it != node.end(); ++it) {
		std::string key = it->first.as<std::string>();
		const YAML::Node& valueNode = it->second;

		// We need to determine the type from the YAML content
		// Since we don't have type information, we'll make educated guesses
		status_t status = _LoadMessageValue(key.c_str(), valueNode, message);
		if (status != B_OK) {
			LogError("_LoadMessage: Failed to load key [%s]", key.c_str());
			return status;
		}
	}

	return B_OK;
}

status_t YamlPSP::_LoadMessageValue(const char* key, const YAML::Node& node, GMessage& message)
{
	try {
		if (node.IsSequence()) {
			// Handle arrays - add each element
			LogInfo("Loading array for key '%s' with %zu elements", key, node.size());
			for (size_t i = 0; i < node.size(); i++) {
				status_t status = _LoadMessageValue(key, node[i], message);
				if (status != B_OK) {
					LogError("Failed to load array element %zu for key '%s'", i, key);
					return status;
				}
			}
			LogInfo("Successfully loaded array for key '%s'", key);
			return B_OK;
		} else if (node.IsMap()) {
			// Nested message
			GMessage subMessage;
			status_t status = _LoadMessage(node, subMessage);
			if (status == B_OK) {
				return message.AddMessage(key, &subMessage);
			}
			return status;
		} else if (node.IsScalar()) {
			// Try to determine the type from the content
			std::string strValue = node.as<std::string>();

			// Check if it's an entry_ref (ref:// prefixed)
			if (strValue.length() > 6 && strValue.substr(0, 6) == "ref://") {
				std::string actualPath = strValue.substr(6);
				entry_ref ref;
				if (get_ref_for_path(actualPath.c_str(), &ref) == B_OK) {
					return message.AddRef(key, &ref);
				}
				// If failed to create ref, fall through to treat as string
			}

			// Check if it's a rectangle format
			if (strValue.length() > 2 && strValue[0] == '[' && strValue[strValue.length()-1] == ']') {
				BRect rect;
				if (_ParseRectFromString(strValue, rect)) {
					return message.AddRect(key, rect);
				}
			}

			// Check if it's a boolean
			if (strValue == "true" || strValue == "false") {
				bool boolValue = (strValue == "true");
				return message.AddBool(key, boolValue);
			}

			// Check if it's a number
			if (strValue.find('.') != std::string::npos) {
				// Float/double
				try {
					double doubleValue = std::stod(strValue);
					return message.AddDouble(key, doubleValue);
				} catch (...) {
					// Fall through to string
				}
			} else {
				// Integer
				try {
					int64 intValue = std::stoll(strValue);
					if (intValue >= INT32_MIN && intValue <= INT32_MAX) {
						return message.AddInt32(key, (int32)intValue);
					} else {
						return message.AddInt64(key, intValue);
					}
				} catch (...) {
					// Fall through to string
				}
			}

			// Default to string
			return message.AddString(key, strValue.c_str());
		}
	} catch (const YAML::Exception& e) {
		LogError("_LoadMessageValue: YAML exception: %s", e.what());
		return B_ERROR;
	} catch (...) {
		LogError("_LoadMessageValue: Unknown exception occurred");
		return B_ERROR;
	}

	return B_ERROR;
}

status_t YamlPSP::_SaveKey(YAML::Node& yaml, const char* key, GMessage& storage, int32 keyIndex)
{
	switch(storage.Type(key)){
		_yaml_case(B_BOOL_TYPE, bool, Bool)
		_yaml_case_string(B_STRING_TYPE, String)
		_yaml_case(B_INT32_TYPE,   int32,  Int32)
		_yaml_case(B_UINT32_TYPE, uint32, UInt32)
		_yaml_case(B_INT64_TYPE,   int64,  Int64)
		_yaml_case(B_UINT64_TYPE, uint64, UInt64)
		_yaml_case(B_INT16_TYPE,  int16,  Int16)
		_yaml_case(B_UINT16_TYPE, uint16, UInt16)
		_yaml_case(B_INT8_TYPE,  int8,  Int8)
		_yaml_case(B_UINT8_TYPE, uint8, UInt8)
		_yaml_case(B_FLOAT_TYPE, float, Float)
		_yaml_case(B_DOUBLE_TYPE, double, Double)
		case B_RECT_TYPE:
		{
			BRect r;
			status_t status = storage.FindRect(key, keyIndex, &r);
			if (status == B_OK) {
				BString yrect("[");
				yrect << r.left  << ", " << r.top    << ", ";
				yrect << r.right << ", " << r.bottom << "]";
				yaml[key] = yrect.String();
			}
			return status;
		}
		case B_MESSAGE_TYPE:
		{
			GMessage msg;
			YAML::Node subnode;
			LogInfo("_SaveKey(MESSAGE, %s, Index %ld) - Message has %d fields", key, keyIndex, msg.CountNames(B_ANY_TYPE));
			status_t st = storage.FindMessage(key, keyIndex, &msg);
			if ( st == B_OK) {
				//parse all the values
				int32 index = 0;
				type_code type = B_ANY_TYPE;
				char* name = nullptr;
				int32 count = 0;
				bool hasFields = false;

				while(msg.GetInfo(B_ANY_TYPE, index++, &name, &type, &count) == B_OK) {
					hasFields = true;
					LogInfo("Processing field %ld: Name [%s] in message [%s], Count %ld", index, name, key, count);

					// Create a safe copy of the field name to avoid issues with recursive calls
					BString safeName(name);
					std::string subkey(safeName.String());
					if (count > 1) {
						// Multiple values - create YAML sequence
						YAML::Node sequenceNode(YAML::NodeType::Sequence);
						for (int32 subIndex = 0; subIndex < count; subIndex++) {
							YAML::Node itemNode;
							if (_SaveKey(itemNode, subkey.c_str(), msg, subIndex) == B_OK) {
								// Extract the actual value from the itemNode
								if (itemNode[subkey]) {
									sequenceNode.push_back(itemNode[subkey]);
								}
							}
						}
						subnode[subkey] = sequenceNode;
					} else if (count == 1) {
						// Single value
						YAML::Node itemNode;
						if (_SaveKey(itemNode, subkey.c_str(), msg, 0) == B_OK) {
							// Extract the actual value from the itemNode
							if (itemNode[subkey]) {
								subnode[subkey] = itemNode[subkey];
							}
						}
					}
				}

				// If the message is empty, write null
				if (!hasFields) {
					yaml[key] = YAML::Node(YAML::NodeType::Null);
				} else {
					yaml[key] = subnode;
				}

				LogInfo("Finished processing BMessage %s", key);
				//msg.PrintToStream();
			}
			return st;
		}
		case B_REF_TYPE:
		{
			entry_ref ref;
			if (storage.FindRef(key, keyIndex, &ref) == B_OK) {
				BPath path(&ref);
				BString refString("ref://");
				refString << path.Path();
				yaml[key] = refString.String();
			}
			break;
		}
		default:
			debugger("Conversion to YAML not supported!");
			break;

	};
	return B_OK;
}
