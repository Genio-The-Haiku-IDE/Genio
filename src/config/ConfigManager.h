#pragma once

#include <Autolock.h>
#include <Application.h>
#include <Path.h>

#include "GMessage.h"

enum StorageType {
	kStorageTypeBMessage  = 0,
	kStorageTypeAttribute = 1,

	kStorageTypeCountNb   = 2
};

class ConfigManagerReturn;
class ConfigManager {
public:
		explicit ConfigManager(const int32 messageWhat);

		template<typename T>
		void AddConfig(const char* group,
		               const char* key,
					   const char* label,
					   T default_value,
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
		}

		status_t	LoadFromFile(BPath messageFilePath, BPath attributeFilePath = BPath());
		status_t	SaveToFile(BPath messageFilePath, BPath attributeFilePath = BPath());

		void ResetToDefaults();
		bool HasAllDefaultValues();

		void PrintAll() const;
		void PrintValues() const;

		auto operator[](const char* key) -> ConfigManagerReturn;

		bool Has(GMessage& msg, const char* key) const;

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

    bool	_CheckKeyIsValid(const char* key) const;
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

