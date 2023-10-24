#pragma once
#include "GMessage.h"
#include <Autolock.h>
class BView;


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
		void operator =(T n) { BAutolock lock(fLocker); fMsg[fKey] = n; };
private:
	GMessage& fMsg;
	const char* fKey;
	BLocker& fLocker;
};

class ConfigManager {

public:
		explicit ConfigManager(){
			fLocker.InitCheck();
		}

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


		void ResetToDefault() {
			GMessage msg;
			int i=0;
			while(configuration.FindMessage("config", i++, &msg) == B_OK) {
				storage[msg["key"]] = msg["default_value"];
			}
		}

		void Print() {
			storage.PrintToStream();
			configuration.PrintToStream();
		}

		auto operator[](const char* key) -> ConfigManagerReturn {
			type_code type;
			if (storage.GetInfo(key, &type) != B_OK) {
				printf("No info for key [%s]\n", key);
				throw new std::exception();
			}
			return ConfigManagerReturn(storage, key, fLocker);
		}

		bool Has(GMessage& msg, const char* key) {
			type_code type;
			return (msg.GetInfo(key, &type) == B_OK);
		}

		GMessage&	Configuration() { return configuration; }

		BView* MakeView();
		//BView* MakeView2();
		BView* MakeViewFor(const char* groupName, GMessage& config);
		BView* MakeSelfHostingViewFor(GMessage& config);
		BView* MakeViewFor(GMessage& config);


protected:
		GMessage storage;
		GMessage configuration;
		BLocker	 fLocker;


};

// TODO: Use a static method ?
extern ConfigManager& gCFG;
