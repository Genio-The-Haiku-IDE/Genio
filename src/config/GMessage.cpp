/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GMessage.h"

void GMessage::_HandleVariantList(variant_list& list) {
	MakeEmpty();
	auto i = list.begin();
	while(i != list.end()) {
		std::visit([&] (const auto& k) {
			(*this)[(*i).key] = k;
		}, (*i).value);
		i++;
	}
}

auto GMessage::operator[](const char* key) -> GMessageReturn { return GMessageReturn(this, key); }