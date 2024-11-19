/*
 * Copyright 2023-2024, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "IconCache.h"

#include <Bitmap.h>
#include <ControlLook.h>
#include <MimeTypes.h>
#include <NodeInfo.h>

#include "Log.h"


IconCache IconCache::sInstance;

IconCache::IconCache()
{
}


const BBitmap*
IconCache::GetIcon(const entry_ref *ref, const float iconSize)
{
	BNode node(ref);
	const BNodeInfo nodeInfo(&node);
	char mimeType[B_MIME_TYPE_LENGTH];
	const char* mimeTypePtr = mimeType;
	if (nodeInfo.GetType(mimeType) != B_OK) {
		LogDebug("Invalid mimeType for file [%s]", ref->name);
		if (node.IsDirectory())
			mimeTypePtr = B_DIRECTORY_MIME_TYPE;
		else
			mimeTypePtr = B_FILE_MIME_TYPE;
	}

	LogTrace("IconCache: [%s] - [%s]", mimeTypePtr, ref->name);

	auto cache = sInstance.fCaches.find(iconSize);
	if (cache == sInstance.fCaches.end()) {
		sInstance.fCaches[iconSize] = new bitmap_cache;
		cache = sInstance.fCaches.find(iconSize);
	}
	auto it = cache->second->find(mimeTypePtr);
	if (it != cache->second->end()) {
		LogTrace("IconCache: return icon from cache for %s", mimeTypePtr);
		return it->second;
	} else {
		LogTrace("IconCache: could not find an icon in cache for %s", mimeTypePtr);
		const BRect rect(0, 0, iconSize - 1, iconSize - 1);
		BBitmap *icon = new BBitmap(rect, B_RGBA32);
		status_t status = nodeInfo.GetTrackerIcon(icon, icon_size(iconSize));
		cache->second->emplace(mimeTypePtr, icon);
		LogTrace("IconCache: GetTrackerIcon returned - %s", ::strerror(status));
		return icon;
	}
	return nullptr;
}


const BBitmap*
IconCache::GetIcon(const BString& path, const float iconSize)
{
	const BEntry entry(path);
	entry_ref ref;
	entry.GetRef(&ref);
	return IconCache::GetIcon(&ref, iconSize);
}


void
IconCache::PrintToStream()
{
#if 0
	printf("IconCache %p: cache content\n", &sInstance);
	for (auto const& x : sInstance.fCache) {
		printf("IconCache: %s\n", x.first.c_str());
	}
	printf("----------------------------\n");
#endif
}
