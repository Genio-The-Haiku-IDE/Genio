#pragma once

#include <Autolock.h>
#include <Application.h>

#include "GMessage.h"

class ConfigManagerReturn;
class ConfigManager {
public:
		explicit ConfigManager(const int32 messageWhat);

		template<typename T>
		void AddConfig(const char* group, const char* key, const char* label, T default_value, GMessage* cfg = nullptr) {
			GMessage configKey;
			if (cfg)
				configKey = *cfg;

			configKey["group"]			= group;
			configKey["key"]			= key;
			configKey["label"]    		= label;
			configKey["default_value"]  = default_value;
			configKey["type_code"] 		= MessageValue<T>::Type();

			storage[key] = default_value;

			configuration.AddMessage("config", &configKey);
		}

		status_t	LoadFromFile(BPath path);
		status_t	SaveToFile(BPath path);

		void ResetToDefaults();
		bool HasAllDefaultValues();

		void PrintAll() const;
		void PrintValues() const;

		auto operator[](const char* key) -> ConfigManagerReturn;

		bool Has(GMessage& msg, const char* key) const;

		GMessage&	Configuration() { return configuration; }

		int32 UpdateMessageWhat() const { return fWhat; }
		BLocker& Locker() { return fLocker; }

		void SendNotifications(BMessage* message);

protected:
		GMessage storage;
		GMessage configuration;
		BLocker	 fLocker;
		int32	 fWhat;
private:
		bool	_SameTypeAndFixedSize(BMessage* msgL, const char* keyL,
									  BMessage* msgR, const char* keyR) const;
};


class ConfigManagerReturn {
public:
		ConfigManagerReturn(GMessage& msg, const char* key, ConfigManager& config)
			:
			fMsg(msg),
			fKey(key),
			fConfigManager(config) {
		}

		template< typename Return >
        operator Return() {
			BAutolock lock(fConfigManager.Locker());
			return fMsg[fKey];
		};

		template< typename T >
		void operator =(T n) {
			BAutolock lock(fConfigManager.Locker());
			fMsg[fKey] = n;
			GMessage noticeMessage(fConfigManager.UpdateMessageWhat());
			noticeMessage["key"]  	= fKey;
			noticeMessage["value"]  = fMsg[fKey];
			fConfigManager.SendNotifications(&noticeMessage);
		};
private:
	GMessage& fMsg;
	const char* fKey;
	ConfigManager& fConfigManager;
};
