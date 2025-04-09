/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <String.h>
#include <SupportDefs.h>
#include <Message.h>
#include <cassert>
#include <typeinfo>
#include <vector>

class BMessageBlob {
public:
	virtual status_t	Serialize(BMessage* msg) const = 0;
	virtual status_t	Deserialize(const BMessage* msg) = 0;
};


template <typename T>
class BlobVector : public std::vector<T>, public BMessageBlob {

public:

    // Check if T is a subclass of BMessageBlob or if T is a pointer to a subclass of BMessageBlob
    static_assert(
        std::is_base_of<BMessageBlob, T>::value ||
        (std::is_pointer<T>::value && std::is_base_of<BMessageBlob, typename std::remove_pointer<T>::type>::value),
        "T must be a subclass of BMessageBlob or a pointer to a subclass of BMessageBlob"
    );
	status_t	Serialize(BMessage* msg) const override {

		if (msg == nullptr)
			return B_ERROR;

		BString type;
		if constexpr (std::is_pointer<T>::value) {
			type = typeid(typename std::remove_pointer<T>::type).name();
		} else {
			type = typeid(T).name();
		}

		for (const auto& item : *this) {
			BMessage xyx;
			if constexpr (std::is_pointer<T>::value) {
				item->Serialize(&xyx);
			} else {
				item.Serialize(&xyx);
			}
			msg->AddMessage(type, &xyx);
        }
		msg->PrintToStream();
		debugger("DONE");
		return B_OK;
	}
	status_t	Deserialize(const BMessage* msg) override {
		return B_OK;
	}
};


