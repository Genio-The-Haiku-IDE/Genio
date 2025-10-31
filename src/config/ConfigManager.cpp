/*
 * Copyright 2018-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ConfigManager.h"

#include <File.h>
#include <Path.h>
#include <Entry.h>
#include <sstream>
#include <vector>

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
//#define _yaml_case(B_BOOL_TYPE, bool, Bool)
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
#define _yaml_load(TYPE, pod) \
				case TYPE: \
				{ 	storage[key] = node.as<pod>(); return B_OK; }


#include <yaml-cpp/yaml.h>
class YamlPSP : public PermanentStorageProvider {
public:
		YamlPSP()
		{
		}

		virtual status_t Open(const BPath& _dest, kPSPMode mode)
		{
			yaml.reset();
			//workaround.
			BPath dest;
			dest.SetTo(BString(_dest.Path()).Append(".yaml").String());

			uint32 fileMode = mode == PermanentStorageProvider::kPSPReadMode ? B_READ_ONLY : (B_WRITE_ONLY | B_CREATE_FILE);
			status_t status = fFile.SetTo(dest.Path(), fileMode);
			if (status == B_OK && fFile.IsReadable()) {
				try {
					yaml = YAML::LoadFile(dest.Path());
					LogInfo("YAML file loaded successfully: %s", dest.Path());
				} catch(const YAML::ParserException& e) {
					LogError("YAML Parser Error in file %s at line %zu, column %zu: %s",
						dest.Path(), e.mark.line + 1, e.mark.column + 1, e.msg.c_str());
					return B_ERROR;
				} catch(const YAML::BadFile& e) {
					LogError("YAML Bad File Error: Cannot open file %s: %s",
						dest.Path(), e.msg.c_str());
					return B_ERROR;
				} catch(const YAML::Exception& e) {
					LogError("YAML General Error in file %s: %s",
						dest.Path(), e.msg.c_str());
					return B_ERROR;
				} catch(const std::exception& e) {
					LogError("Standard Exception while loading YAML file %s: %s",
						dest.Path(), e.what());
					return B_ERROR;
				} catch(...) {
					LogError("Unknown exception while loading YAML file %s", dest.Path());
					return B_ERROR;
				}
			} else if (status != B_OK) {
				LogError("Failed to open file %s with mode %s: %s",
					dest.Path(),
					(mode == kPSPReadMode) ? "READ" : "WRITE",
					::strerror(status));
			}
			return status;
		}

		virtual status_t Close()
		{
			status_t status = fFile.InitCheck();
			if (status == B_OK && fFile.IsWritable()) {

				YAML::Emitter out;
				out << yaml;
				BString bout(out.c_str());

				fFile.Write(bout.String(), bout.Length());

				status = fFile.Flush();
				fFile.Unset();
				return status;
			}
			return status;
		}

		virtual status_t LoadKey(ConfigManager& manager, const char* key,
								GMessage& storage, GMessage& parameterConfig)
		{
			const YAML::Node fromFile = yaml[key];
			if (!fromFile) {
				return B_NAME_NOT_FOUND;
			}

			// Get the expected type from the parameter config
			type_code expectedType = parameterConfig.Type("default_value");
			if (expectedType == 0) {
				LogError("LoadKey: No default_value type found for key [%s]", key);
				return B_ERROR;
			}

			// Remove any existing data for this key
			storage.RemoveName(key);

			status_t status = B_OK;

			/*if (fromFile.IsSequence()) {
				// Handle arrays/sequences
				for (size_t i = 0; i < fromFile.size(); i++) {
					debugger("Sequence");
					LogError("Sequence array for key [%s]", key);
					status = _LoadSingleValue(fromFile[i], key, storage, expectedType);
					if (status != B_OK) {
						LogError("LoadKey: Failed to load array element %zu for key [%s]", i, key);
						break;
					}
				}
			} else {*/
				// Handle single values
				status = _LoadSingleValue(fromFile, key, storage, expectedType);
			//}


			LogError("->Loaded key %s", key);
			return status;
		}

		virtual status_t SaveKey(ConfigManager& manager, const char* key, GMessage& storage)
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
private:
		YAML::Node yaml;
		BFile fFile;

		status_t _LoadSingleValue(const YAML::Node& node, const char* key, GMessage& storage, type_code expectedType)
		{
			try {
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
						if (node.IsNull() == false && node.IsMap()) {
							GMessage subMessage;
							LogError("Message for key %s\n", key);
							status_t status = _LoadMessage(node, subMessage);
							if (status == B_OK) {
								storage[key] = subMessage;
								subMessage.PrintToStream();
								return B_OK;
							}
							return status;
						}
						storage[key] = GMessage();
						LogError("_LoadSingleValue: Expected map for B_MESSAGE_TYPE null:%d map:%d", node.IsNull(), node.IsMap());
						return B_ERROR;
					}
					case B_REF_TYPE:
					{
						std::string pathStr = node.as<std::string>();
						entry_ref ref;
						if (get_ref_for_path(pathStr.c_str(), &ref) == B_OK) {
							storage[key] = ref;
							return B_OK;
						}
						LogError("_LoadSingleValue: Failed to create entry_ref for path: %s", pathStr.c_str());
						return B_ERROR;
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

		bool _ParseRectFromString(const std::string& rectStr, BRect& rect)
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

		status_t _LoadMessage(const YAML::Node& node, GMessage& message)
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

		status_t _LoadMessageValue(const char* key, const YAML::Node& node, GMessage& message)
		{
			try {
				if (node.IsSequence()) {
					// Handle arrays - add each element
					for (size_t i = 0; i < node.size(); i++) {
						status_t status = _LoadMessageValue(key, node[i], message);
						if (status != B_OK) {
							return status;
						}
					}
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

		status_t _SaveKey(YAML::Node& yaml, const char* key, GMessage& storage, int32 keyIndex)
		{
			switch(storage.Type(key)){
				_yaml_case(B_BOOL_TYPE, bool, Bool)
				_yaml_case(B_STRING_TYPE,   const char*,  String)
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
					LogError("_SaveKey(MESSAGE, %s, Index %ld)", key, keyIndex);
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
							LogError("Index %ld Name [%s][%s] Count %ld", index, key, name, count);

							std::string subkey(name);
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

						LogError("Count %ld %s", count, key);
						//msg.PrintToStream();
					}
					return st;
				}
				case B_REF_TYPE:
				{
					entry_ref ref;
					if (storage.FindRef(key, keyIndex, &ref) == B_OK) {
						BPath path(&ref);
						yaml[key] = path.Path();
					}
					break;
				}
				default:
					debugger("Conversion to YAML not supported!");
					break;

			};
			return B_OK;
		}
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
			return new YamlPSP();
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
	fNoticeMessage.RemoveData(kContext);
	fNoticeMessage.AddString(kContext, "load_from_file");

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
			fNoticeMessage.ReplaceString(kContext, "load_from_file_end");

		status = provider->LoadKey(*this, key, fStorage, msg);
		if (status == B_OK) {
			LogInfo("Config file: loaded value for key [%s] (StorageType %d)", key, storageType);
		} else {
			LogError("Config file: unable to get valid key [%s] (%s) (StorageType %d)",
				key, ::strerror(status), storageType);
		}
	}

	fNoticeMessage.RemoveData(kContext);

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

	fNoticeMessage.RemoveData(kContext);
	fNoticeMessage.AddString(kContext, "reset_to_defaults");
	while (fConfiguration.FindMessage("config", i++, &msg) == B_OK) {
		if (countFound == i)
			fNoticeMessage.ReplaceString(kContext, "reset_to_defaults_end");

		fStorage[msg["key"]] = msg["default_value"]; //to force the key creation
		(*this)[msg["key"]] = msg["default_value"]; //to force the update
	}

	fNoticeMessage.RemoveData(kContext);
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
