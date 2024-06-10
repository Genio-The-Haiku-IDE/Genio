#pragma once

#include <Autolock.h>
#include <Application.h>
#include <Path.h>
#include <array>

#include "GMessage.h"

enum StorageType {
	kStorageTypeBMessage  = 0,
	kStorageTypeAttribute = 1,
	kStorageTypeNoStore	  = 2,

	kStorageTypeCountNb   = 3
};

class PermanentStorageProvider;
class ConfigManagerReturn;
class ConfigManager {
public:
		explicit ConfigManager(const int32 messageWhat);
				 ~ConfigManager();

		void AddConfigSeparator(const char* group,
						  const char* key,
						  const char* label) {

			GMessage configKey;
			configKey["group"]			= group;
			configKey["key"]			= key;
			configKey["label"]    		= label;
			configKey["default_value"]  = "separator";
			configKey["type_code"] 		= (type_code)B_OBJECT_TYPE;
			configKey["storage_type"]	= (int32)kStorageTypeNoStore;

			fStorage[key] = "separator";

			fConfiguration.AddMessage("config", &configKey);

			if (fPSPList[(int32)kStorageTypeNoStore] == nullptr)
				fPSPList[(int32)kStorageTypeNoStore] = CreatePSPByType(kStorageTypeNoStore);
		}

		template<typename T>
		void AddConfig(const char* group,
		               const char* key,
					   const char* label,
					   T defaultValue,
					   GMessage* cfg = nullptr,
					   StorageType storageType = kStorageTypeBMessage) {

			GMessage configKey;
			if (cfg)
				configKey = *cfg;

			configKey["group"]			= group;
			configKey["key"]			= key;
			configKey["label"]    		= label;
			configKey["default_value"]  = defaultValue;
			configKey["type_code"] 		= MessageValue<T>::Type();
			configKey["storage_type"]	= (int32)storageType;

			fStorage[key] = defaultValue;

			fConfiguration.AddMessage("config", &configKey);

			if (fPSPList[(int32)storageType] == nullptr)
				fPSPList[(int32)storageType] = CreatePSPByType(storageType);
		}

		status_t	SaveToFile(std::array<BPath, kStorageTypeCountNb> paths);
		status_t	LoadFromFile(std::array<BPath, kStorageTypeCountNb> paths);

		void ResetToDefaults();
		bool HasAllDefaultValues();

		void PrintAll() const;
		void PrintValues() const;

		auto operator[](const char* key) -> ConfigManagerReturn;

		bool HasKey(const char* key) const;

		GMessage& Configuration() { return fConfiguration; }

		int32 UpdateMessageWhat() const { return fWhat; }

private:
friend ConfigManagerReturn;

		template< typename Return >
		Return get(const char* key)
		{
			BAutolock lock(fLocker);
			return fStorage[key];
		}

		template< typename T >
		void set(const char* key, T n)
		{
			BAutolock lock(fLocker);
			if (!_CheckKeyIsValid(key))
				return;
			fStorage[key] = n;
			GMessage noticeMessage(fWhat);
			noticeMessage["key"]  	= key;
			noticeMessage["value"]  = fStorage[key];
			if (be_app != nullptr)
				be_app->SendNotices(fWhat, &noticeMessage);
		}

private:
		GMessage fStorage;
		GMessage fConfiguration;
		BLocker	 fLocker;
		int32	 fWhat;
		PermanentStorageProvider*	fPSPList[kStorageTypeCountNb];

    bool	_CheckKeyIsValid(const char* key) const;
	PermanentStorageProvider*	CreatePSPByType(StorageType type);
};


class ConfigManagerReturn {
public:
		ConfigManagerReturn(const char* key, ConfigManager& manager):
			fKey(key),
			fConfigManager(manager) {
		}

		template< typename Return >
        operator Return() { return fConfigManager.get<Return>(fKey);  };

		template< typename T >
		void operator =(T n) { fConfigManager.set<T>(fKey, n); };
private:
	const char* fKey;
	ConfigManager& fConfigManager;
};

