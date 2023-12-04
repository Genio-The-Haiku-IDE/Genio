/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GMessage.h"

void GMessage::_HandleVariantList(variant_list& list) {
	MakeEmpty();
    for (auto &[k, v]: list)
    {
		std::visit([&] (const auto& z) {
			(*this)[k.c_str()] = z;
		}, *v);
    }
}

auto GMessage::operator[](const char* key) -> GMessageReturn { return GMessageReturn(this, key); }

template<>
bool
GMessageReturn::is_what<int>(int w) { if (strcmp(fKey, "what") == 0) {fMsg->what = w; return true;} return false; }

