/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 // Version 2
 // Requires C++17
 //
 // Example:
 //   GMessage msg;
 //   msg["name"] = "Andrea";
 //   msg["points"] = 1412;
 //   msg["active"] = true;
 //
 //   printf("Points: %d\n", (int32)msg["points"]);
 //
 // brace-enclosed list:
 //  GMessage msg2 = { {"name", "Andrea"}, {"points", 1412}, {"active", true} };
 //
 // submessages:
 //	 GMessage msg3 = msg2;
 //	 msg["details"] = { {"id", 442}, {"cost", 78} };
 //
 //	GMessage levels3 = {
 //		{"what", 'HOLA' },
 //		{"mode", "options"},
 //		{"option_1", {
 //			{"what", 'HERE' },
 //			{"value", (int32)0 },
 //			{"label", "Off" }}}
 //	};
 //
 // NOTE: the 'what' key with an int32 value, sets the message->what field!

#pragma once
#include <Message.h>
#include <String.h>
#include <cstring>
#include <memory>
#include <string>
#include <variant>

#define KV(T) kv_pair(const std::string &k, T v) : pair(k, std::make_shared<mapped_type>(v)) {}

#define SUPPORTED_1		int32, bool, const char*, BString, GMessage
#define SUPPORTED_2		KV(int32); KV(bool); KV(const char*); KV(BString); KV(GMessage);

class GMessageReturn;
class GMessage : public BMessage {
public:

  using mapped_type = std::variant<SUPPORTED_1>;
  struct kv_pair : public std::pair<std::string, std::shared_ptr<mapped_type>> { SUPPORTED_2 };
  using variant_list = std::initializer_list<kv_pair>;


	explicit GMessage():BMessage() {};
	explicit GMessage(uint32 what):BMessage(what){};
	GMessage(variant_list n) { _HandleVariantList(n); };

	auto operator[](const char* key) -> GMessageReturn;

	bool Has(const char* key) const {
		type_code type;
		return (GetInfo(key, &type) == B_OK);
	}

	type_code Type(const char* key) const {
		type_code type;
		return (GetInfo(key, &type) == B_OK ? type : B_ANY_TYPE);
	}

private:
	void _HandleVariantList(variant_list& list);
};


template<typename T>
class MessageValue {
public:
	static T	Get(GMessage* msg, const char* key){
					T write_specific_converter;
					return Unsupported_GetValue_for_GMessage(write_specific_converter);
	}
	static void	Set(GMessage* msg, const char* key, T value){
					T write_specific_converter;
					Unsupported_SetValue_for_GMessage(write_specific_converter);
	}
	static type_code	Type();
};


#define MESSAGE_VALUE(NAME, TYPE, typeCODE, DEFAULT) \
template<> \
class MessageValue<TYPE> { \
public: \
	static TYPE		Get(GMessage* msg, const char* key) { return msg->Get ## NAME (key, DEFAULT); } \
	static void		Set(GMessage* msg, const char* key, TYPE value) { msg->Set ## NAME (key, value); } \
	static type_code	Type() { return typeCODE; } \
}; \


#define MESSAGE_VALUE_REF(NAME, TYPE, typeCODE) \
template<> \
class MessageValue<TYPE> { \
public: \
	static TYPE		Get(GMessage* msg, const char* key) { \
						TYPE value; \
						msg->Find ## NAME (key, &value); \
						return value; } \
	static void		Set(GMessage* msg, const char* key, TYPE value) { \
						msg->RemoveName(key); \
						msg->Add ## NAME (key, &value); } \
	static type_code	Type() { return typeCODE; } \
};

MESSAGE_VALUE(Bool, bool, B_BOOL_TYPE, false);
MESSAGE_VALUE(String, const char*, B_STRING_TYPE, "");
MESSAGE_VALUE(Int32, int32, B_INT32_TYPE, -1);

#if __HAIKU_BEOS_COMPATIBLE_TYPES
	// on Haiku x86_32 bit, due to this define, int32 is defined as
	// "signed long int" instead of "signed int" like on other platforms
	// so we need this extra line to handle "int".
MESSAGE_VALUE(Int32, int, B_INT32_TYPE, -1);
#endif

MESSAGE_VALUE(UInt32, uint32, B_UINT32_TYPE, 0);
MESSAGE_VALUE(Rect, BRect, B_RECT_TYPE, BRect());
MESSAGE_VALUE(String, BString, B_STRING_TYPE, BString(""));
MESSAGE_VALUE_REF(Message, GMessage, B_MESSAGE_TYPE);
MESSAGE_VALUE_REF(Message, BMessage, B_MESSAGE_TYPE);

class GMessageReturn {
public:
		GMessageReturn(GMessage* msg, const char* key, GMessageReturn* parent = nullptr) {
			fMsg=msg; fKey=key; fSyncParent = parent;
		}

		 ~GMessageReturn() {
			if (fSyncParent) {
				(*fSyncParent) = (*fMsg);
				delete fMsg;
			}
		 }

		template< typename Return >
        operator Return() { return MessageValue<Return>::Get(fMsg, fKey); };

		template< typename T >
		void operator=(T n) { if (!is_what(n)) MessageValue<T>::Set(fMsg, fKey, n); }

		template<typename T>
		bool is_what(T n) { return false; }


		void operator=(GMessage::variant_list n){
			GMessage xmsg(n);
			MessageValue<GMessage>::Set(fMsg, fKey, xmsg);
		}

		auto operator[](const char* key) -> GMessageReturn {
			GMessage* newMsg = new GMessage();
			fMsg->FindMessage(fKey, newMsg);
			return GMessageReturn(newMsg, key, this);
		};

		void operator=(const GMessageReturn& n) {
			type_code typeFound;
			bool fixedSize;
			if (fMsg == n.fMsg && strcmp(n.fKey, fKey) == 0)
				return;

			if (n.fMsg->GetInfo(n.fKey, &typeFound, &fixedSize) == B_OK) {
				const void* data = nullptr;
				ssize_t numBytes = 0;
				if (n.fMsg->FindData(n.fKey, typeFound, &data, &numBytes) == B_OK) {
					fMsg->RemoveData(fKey); //remove the key
					fMsg->SetData(fKey, typeFound, data, numBytes, fixedSize);
				}
			}
		}

		bool operator !=(const GMessageReturn& n) {
			return !operator==(n);
		}

		bool operator ==(const GMessageReturn& n) {
			type_code typeLeft;
			type_code typeRight;
			const void* dataLeft = nullptr;
			const void* dataRight = nullptr;
			ssize_t numBytesLeft = 0;
			ssize_t numBytesRight = 0;

			bool comparison = false;
			if (n.fMsg->GetInfo(n.fKey, &typeRight) == B_OK &&
				  fMsg->GetInfo(fKey, &typeLeft) == B_OK) {

				if (typeLeft == typeRight &&
					n.fMsg->FindData(n.fKey, typeRight, &dataRight, &numBytesRight) == B_OK &&
					  fMsg->FindData(fKey, typeLeft, &dataLeft, &numBytesLeft) == B_OK) {

					if (numBytesLeft == numBytesRight) {
						comparison = (memcmp(dataRight, dataLeft, numBytesRight) == 0);
					}
				}
			}

			return comparison;
		}

		void Print() const {
			fMsg->PrintToStream();
		}
private:
		GMessage* fMsg;
		const char* fKey;
		GMessageReturn* fSyncParent;
};

// Heap Message, deprecated!
#define SMSG(WHAT, LIST...) new GMessage({{"what",WHAT}, LIST})

//BMessage wrapper
#define BMSG(in, out) GMessage& out = *((GMessage*)in);
