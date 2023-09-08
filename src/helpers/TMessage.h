/*
 * Copyright 2018, Your Name 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <any>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include <Alignment.h>
#include <StringList.h>
#include <Message.h>
#include <Messenger.h>
#include <SupportDefs.h>

namespace Genio::Message {

	using namespace std;

	typedef const void* Pointer;

	static map<string, type_code> TypeMap = {
		{typeid(BAlignment).name(), B_ALIGNMENT_TYPE},
		{typeid(BRect).name(), B_RECT_TYPE},
		{typeid(BPoint).name(), B_POINT_TYPE},
		{typeid(BSize).name(), B_SIZE_TYPE},
		{typeid(const char *).name(), B_STRING_TYPE},
		{typeid(BString).name(), B_STRING_TYPE},
		{typeid(std::string).name(), B_STRING_TYPE},
		{typeid(BStringList).name(), B_STRING_LIST_TYPE},
		{typeid(int8).name(), B_INT8_TYPE},
		{typeid(int16).name(), B_INT16_TYPE},
		{typeid(int32).name(), B_INT32_TYPE},
		{typeid(int64).name(), B_INT64_TYPE},
		{typeid(uint8).name(), B_UINT8_TYPE},
		{typeid(uint16).name(), B_UINT16_TYPE},
		{typeid(uint32).name(), B_UINT32_TYPE},
		{typeid(uint64).name(), B_UINT64_TYPE},
		{typeid(bool).name(), B_BOOL_TYPE},
		{typeid(float).name(), B_FLOAT_TYPE},
		{typeid(double).name(), B_DOUBLE_TYPE},
		{typeid(rgb_color).name(), B_RGB_COLOR_TYPE},
		{typeid(Pointer).name(), B_POINTER_TYPE},
		{typeid(BMessenger).name(), B_MESSENGER_TYPE},
		{typeid(entry_ref).name(), B_REF_TYPE},
		{typeid(node_ref).name(), B_NODE_REF_TYPE},
		{typeid(BMessage).name(), B_MESSAGE_TYPE},
	};

	

	class TMessage : public BMessage {
	public:
							TMessage() : BMessage() {};
							TMessage(uint32 what) : BMessage(what) {};
							TMessage(const BMessage& other) : BMessage(other) {};
		virtual				~TMessage();

			
		/*
		template <typename Type>
		status_t			Add(const char* name, Type value) {	
								if (is_same_v<Type, BAlignment>)
									return AddAlignment(name, (const Type&)value);
								if (is_same_v<Type, BRect>)
									return AddRect(name, value);
								if (is_same_v<Type, BPoint>)
									return AddPoint(name, value);
								if (is_same_v<Type, BSize>)
									return AddSize(name, value);
								if (is_same_v<Type, const char*>)
									return AddString(name, value);
								if (is_same_v<Type, BString>)
									return AddString(name, (const Type&)value);
								// if (is_same_v<Type, string>)
									// return AddString(name, ((const string&)value).c_str());
								if (is_same_v<Type, BStringList>)
									return AddStrings(name, (const Type&)value);
								if (is_same_v<Type, int8>)
									return AddInt8(name, value);
								if (is_same_v<Type, int16>)
									return AddInt16(name, value);
								if (is_same_v<Type, int32>)
									return AddInt32(name, value);
								if (is_same_v<Type, int64>)
									return AddInt64(name, value);
								if (is_same_v<Type, uint8>)
									return AddUInt8(name, value);
								if (is_same_v<Type, uint16>)
									return AddUInt16(name, value);
								if (is_same_v<Type, uint32>)
									return AddUInt32(name, value);
								if (is_same_v<Type, uint64>)
									return AddUInt64(name, value);
								if (is_same_v<Type, bool>)
									return AddBool(name, value);
								if (is_same_v<Type, float>)
									return AddFloat(name, value);
								if (is_same_v<Type, double>)
									return AddDouble(name, value);
								if (is_same_v<Type, rgb_color>)
									return AddColor(name, value);
								if (is_same_v<Type, Pointer>)
									return Add(name, (Pointer)value);
								if (is_same_v<Type, BMessenger>)
									return AddMessenger(name, value);
								if (is_same_v<Type, entry_ref>)
									return AddRef(name, (const Type*)value);
								if (is_same_v<Type, node_ref>)
									return AddNodeRef(name, (const Type*)value);
								if (is_same_v<Type, BMessage>)
									return AddMessage(name, (const Type*)value);
								
								// if (typeid(Type) == typeid(const BAlignment&))
									// return AddAlignment(name, value); 
								// if (typeid(Type) == typeid(BRect))
									// return AddRect(name, value); 
								// if (typeid(Type) == typeid(BPoint))
									// return AddPoint(name, value); 
								// if (typeid(Type) == typeid(BSize))
									// return AddSize(name, value); 
								// if (typeid(Type) == typeid(const char*))
									// return AddString(name, value);
								// if (typeid(Type) == typeid(const BString&))
									// return AddString(name, value); 
								// if (typeid(Type) == typeid(const BStringList&))
									// return AddStrings(name, value); 
								// if (typeid(Type) == typeid(int8))
									// return AddInt8(name, value); 
								// if (typeid(Type) == typeid(uint8))
									// return AddUInt8(name, value); 
								// if (typeid(Type) == typeid(int16))
									// return AddInt16(name, value); 
								// if (typeid(Type) == typeid(uint16))
									// return AddUInt16(name, value); 
								// if (typeid(Type) == typeid(int32))
									// return AddInt32(name, value); 
								// if (typeid(Type) == typeid(uint32))
									// return AddUInt32(name, value); 
								// if (typeid(Type) == typeid(int64))
									// return AddInt64(name, value); 
								// if (typeid(Type) == typeid(uint64))
									// return AddUInt64(name, value); 
								// if (typeid(Type) == typeid(bool))
									// return AddBool(name, value); 
								// if (typeid(Type) == typeid(float))
									// return AddFloat(name, value); 
								// if (typeid(Type) == typeid(double))
									// return AddDouble(name, value); 
							}*/
								
		template <typename Type>
		optional<Type>		Get(const char* name, int32 index = 0) {	
								ssize_t size = 0;
								void *result = nullptr;
								optional<Type> return_value;
								type_code type = TypeMap[typeid(Type).name()];
								// if (is_same_v<Type, BStringList>) type = B_STRING_LIST_TYPE;
								
								status_t status = FindData(name, index, type, (const void **)&result, &size);
								if (status == B_OK) {
									
									if (is_same_v<Type, BString>)
										return_value = *(Type*)BString((const char *)result);
									
									return_value = *(Type*)result;
								}
								
								return return_value;
							}
							
		template <typename Type>
		status_t			Set(const char* name, Type value, bool isFixedSize = true, int32 count = 1) {	
								void *data = nullptr;
								type_code type = TypeMap[typeid(Type).name()];
								
								if (is_same_v<Type, string>)
									data = value.c_str();
								if (is_same_v<Type, BString>)
									data = value.String();
									
								if (ReplaceData(name, type, data, sizeof(Type)) == B_OK)
									return B_OK;

								return AddData(name, type, (const void *)value, sizeof(Type), isFixedSize, count);
							}
							
						
		template <typename Type>
		status_t			Add(const char* name, Type &value) {	
								void *data = nullptr;
								type_code type = TypeMap[typeid(Type).name()];
								
								// if (is_same_v<Type, string>)
									// data = (void*)(value.c_str());
								// if (is_same_v<Type, BString>)
									// data = (void*)(BString(value));
									
								data = &value;
									
								return AddData(name, type, value, sizeof(Type));
							}
							
		template <typename Type>
		status_t			Replace(const char* name, Type value) {	
								void *data = nullptr;
								type_code type = TypeMap[typeid(Type).name()];
								
								if (is_same_v<Type, string>)
									data = value.c_str();
								if (is_same_v<Type, BString>)
									data = value.String();
									
								if (ReplaceData(name, type, data, sizeof(Type)) == B_OK)
									return B_OK;
							}
							
		template <typename Type>
		bool				Has(const char* name, int32 index = 0) {	
								type_code type = TypeMap[typeid(Type).name()];
								return HasData(name, type, index);
							}
			
		/*
		template <typename Type>
		status_t			Find(const char* name, Type *value) const {
								if (is_same_v<Type, BAlignment>)
									return FindAlignment(name, (Type*)value);
								if (is_same_v<Type, BRect>)
									return FindRect(name, (Type*)value);
								if (is_same_v<Type, BPoint>)
									return FindPoint(name, (Type*)value);
								if (is_same_v<Type, BSize>)
									return FindSize(name, (Type*)value);
								if (is_same_v<Type, const char*>)
									return FindString(name, (Type*)value);
								if (is_same_v<Type, BString>)
									return FindString(name, (Type*)value);
								// if (is_same_v<Type, string>)
									// return FindString(name, ((const string&)value).c_str());
								if (is_same_v<Type, BStringList>)
									return FindStrings(name, (Type*)value);
								if (is_same_v<Type, int8>)
									return FindInt8(name, (Type*)value);
								if (is_same_v<Type, int16>)
									return FindInt16(name, (Type*)value);
								if (is_same_v<Type, int32>)
									return FindInt32(name, (Type*)value);
								if (is_same_v<Type, int64>)
									return FindInt64(name, (Type*)value);
								if (is_same_v<Type, uint8>)
									return FindUInt8(name, (Type*)value);
								if (is_same_v<Type, uint16>)
									return FindUInt16(name, (Type*)value);
								if (is_same_v<Type, uint32>)
									return FindUInt32(name, (Type*)value);
								if (is_same_v<Type, uint64>)
									return FindUInt64(name, (Type*)value);
								if (is_same_v<Type, bool>)
									return FindBool(name, (Type*)value);
								if (is_same_v<Type, float>)
									return FindFloat(name, (Type*)value);
								if (is_same_v<Type, double>)
									return FindDouble(name, (Type*)value);
								if (is_same_v<Type, rgb_color>)
									return FindColor(name, (Type*)value);
								if (is_same_v<Type, Pointer>)
									return Find(name, (Type*)value);
								if (is_same_v<Type, BMessenger>)
									return FindMessenger(name, (Type*)value);
								if (is_same_v<Type, entry_ref>)
									return FindRef(name, (Type*)value);
								if (is_same_v<Type, node_ref>)
									return FindNodeRef(name, (Type*)value);
								if (is_same_v<Type, BMessage>)
									return AddMessage(name, (Type*)value);
							}
							
		template <typename Type>
		status_t			Find(const char* name, int32 index, Type *value) const {
								if (is_same_v<Type, BAlignment>)
									return FindAlignment(name, index, (Type*)value);
								if (is_same_v<Type, BRect>)
									return FindRect(name, index, (Type*)value);
								if (is_same_v<Type, BPoint>)
									return FindPoint(name, index, (Type*)value);
								if (is_same_v<Type, BSize>)
									return FindSize(name, index, (Type*)value);
								if (is_same_v<Type, const char*>)
									return FindString(name, index, (Type*)value);
								if (is_same_v<Type, BString>)
									return FindString(name, index, (Type*)value);
								// if (is_same_v<Type, string>)
									// return FindString(name, ((const string&)value).c_str());
								if (is_same_v<Type, BStringList>)
									return FindStrings(name, index, (Type*)value);
								if (is_same_v<Type, int8>)
									return FindInt8(name, index, (Type*)value);
								if (is_same_v<Type, int16>)
									return FindInt16(name, index, (Type*)value);
								if (is_same_v<Type, int32>)
									return FindInt32(name, index, (Type*)value);
								if (is_same_v<Type, int64>)
									return FindInt64(name, index, (Type*)value);
								if (is_same_v<Type, uint8>)
									return FindUInt8(name, index, (Type*)value);
								if (is_same_v<Type, uint16>)
									return FindUInt16(name, index, (Type*)value);
								if (is_same_v<Type, uint32>)
									return FindUInt32(name, index, (Type*)value);
								if (is_same_v<Type, uint64>)
									return FindUInt64(name, index, (Type*)value);
								if (is_same_v<Type, bool>)
									return FindBool(name, index, (Type*)value);
								if (is_same_v<Type, float>)
									return FindFloat(name, index, (Type*)value);
								if (is_same_v<Type, double>)
									return FindDouble(name, index, (Type*)value);
								if (is_same_v<Type, rgb_color>)
									return FindColor(name, index, (Type*)value);
								if (is_same_v<Type, Pointer>)
									return Find(name, index, (Type*)value);
								if (is_same_v<Type, BMessenger>)
									return FindMessenger(name, index, (Type*)value);
								if (is_same_v<Type, entry_ref>)
									return FindRef(name, index, (Type*)value);
								if (is_same_v<Type, node_ref>)
									return FindNodeRef(name, index, (Type*)value);
								if (is_same_v<Type, BMessage>)
									return AddMessage(name, (Type*)value);
							}*/
							

	};

}