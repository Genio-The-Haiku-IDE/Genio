/*
 * Copyright 2023-2024, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <string>
#include <unordered_map>

#include <String.h>

struct entry_ref;

class BBitmap;
class IconCache {
public:
	IconCache(IconCache const&) = delete;
	void operator=(IconCache const&) = delete;

	static const BBitmap* GetIcon(const entry_ref *ref, const float iconSize);
	static const BBitmap* GetIcon(const BString& path, const float iconSize);
	static void 	PrintToStream();

private:
	IconCache();

	typedef std::unordered_map<std::string, BBitmap*> bitmap_cache;
	std::unordered_map<float, bitmap_cache*> fCaches;

	static IconCache sInstance;
};
