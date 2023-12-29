/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ICON_CACHE_H
#define ICON_CACHE_H

#include <string>
#include <unordered_map>

#include <String.h>

struct entry_ref;

class BBitmap;
class IconCache {
public:
	IconCache(IconCache const&) = delete;
	void operator=(IconCache const&) = delete;

	static const BBitmap* GetIcon(const entry_ref *ref);
	static const BBitmap* GetIcon(const BString& path);
	static void 	PrintToStream();

private:
	IconCache();

	std::unordered_map<std::string, BBitmap*> fCache;

	static IconCache sInstance;
};

#endif // ICON_CACHE_H
