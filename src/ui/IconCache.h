/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ICON_CACHE_H
#define ICON_CACHE_H

#include <Bitmap.h>
#include <ControlLook.h>
#include <MimeType.h>
#include <NodeInfo.h>
#include <String.h>

#include <map>
#include <unordered_map>
#include <memory>
#include <string>

class IconCache
{
	public:
	

		static BBitmap* GetIcon(entry_ref *ref);
		static BBitmap* GetIcon(BString path);
		static void 	PrintToStream();
		
    private:
		IconCache() {}

		std::unordered_map<std::string, BBitmap*> cache;
		
		static IconCache instance;
		
	public:
		IconCache(IconCache const&) = delete;
		void operator=(IconCache const&) = delete;
};

#endif // ICON_CACHE_H