/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#pragma once

#include <String.h>

#include <map>

class StringFormatter {
public:

	using pair_list = std::pair<BString, BString>;
	std::map<BString, BString> Substitutions;

	StringFormatter() {}
	~StringFormatter() {}

	void MakeEmpty()
	{
		Substitutions.clear();
	}

	BString Replace(BString text)
	{
		for(auto &sub: Substitutions) {
			text.ReplaceAll(sub.first, sub.second);
		}
		return text;
	}

	BString operator<<(BString text)
	{
		return Replace(text);
	}

};
