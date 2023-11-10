#pragma once

#include <Autolock.h>
#include <Application.h>

#include "GMessage.h"

#define MSG_NOTIFY_CONFIGURATION_UPDATED	'noCU'

class ConfigManagerReturn;
class ConfigManager {
public:
		explicit ConfigManager();

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


protected:
		GMessage storage;
		GMessage configuration;
		BLocker	 fLocker;
private:
		bool	_SameTypeAndFixedSize(BMessage* msgL, const char* keyL,
									  BMessage* msgR, const char* keyR) const;
};


class ConfigManagerReturn {
public:
		ConfigManagerReturn(GMessage& msg, const char* key, BLocker& lock):
			fMsg(msg),
			fKey(key),
			fLocker(lock){
		}

		template< typename Return >
        operator Return() { BAutolock lock(fLocker); return fMsg[fKey]; };

		template< typename T >
		void operator =(T n) {
					BAutolock lock(fLocker);
					fMsg[fKey] = n;
					GMessage noticeMessage(MSG_NOTIFY_CONFIGURATION_UPDATED);
					noticeMessage["key"]  	= fKey;
					noticeMessage["value"]  = fMsg[fKey];
					if (be_app != nullptr)
						be_app->SendNotices(MSG_NOTIFY_CONFIGURATION_UPDATED, &noticeMessage);
		};
private:
	GMessage& fMsg;
	const char* fKey;
	BLocker& fLocker;
};
