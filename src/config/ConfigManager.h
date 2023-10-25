#pragma once
#include "GMessage.h"
#include <Autolock.h>
#include <Application.h>
#include "Logger.h"
class BView;

#define MSG_NOTIFY_CONFIGURATION_UPDATED	'noCU'

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
					be_app->SendNotices(MSG_NOTIFY_CONFIGURATION_UPDATED, &noticeMessage);
					//printf("Config update! [%s] -> ", fKey); noticeMessage.PrintToStream();
		};
private:
	GMessage& fMsg;
	const char* fKey;
	BLocker& fLocker;
};

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

			configuration.AddMessage("config", &configKey);
		}
		
		status_t	LoadFromFile(BPath path);
		status_t	SaveToFile(BPath path);


		void ResetToDefault();

		void PrintAll();
		void PrintValues();
		
		auto operator[](const char* key) -> ConfigManagerReturn;

		bool Has(GMessage& msg, const char* key);

		GMessage&	Configuration() { return configuration; }


protected:
		GMessage storage;
		GMessage configuration;
		BLocker	 fLocker;
};

// TODO: Use a static method ?
extern ConfigManager& gCFG;
