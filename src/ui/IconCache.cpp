/*
 * Copyright 2023, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "IconCache.h"
#include "Log.h"

#define DIR_FILETYPE "application/x-vnd.Be-directory"
#define FILE_FILETYPE "application/octet-stream"

IconCache IconCache::instance;

IconCache::IconCache()
{

}


BBitmap*
IconCache::GetIcon(entry_ref *ref)
{	
	BNode node(ref);
	BNodeInfo nodeInfo(&node);
	char mimeType[B_MIME_TYPE_LENGTH];
	mimeType[0] = '\0';
	if (nodeInfo.GetType(mimeType) != B_OK) {
		LogDebug("Invalid mimeType for file [%s]", ref->name);
		if (node.IsDirectory())
			strncpy(mimeType, DIR_FILETYPE, B_MIME_TYPE_LENGTH - 1);
		else
			strncpy(mimeType, FILE_FILETYPE, B_MIME_TYPE_LENGTH - 1);
	}
		
	LogTrace("IconCache: [%s] - [%s]", mimeType, ref->name);
	
	auto it = instance.cache.find(mimeType);
	if (it != instance.cache.end()) {
		LogTrace("IconCache: return icon from cache for %s",mimeType);
		return it->second;
	} else {
		LogTrace("IconCache: could not find an icon in cache for %s",mimeType);
		BRect rect(BPoint(0, 0), be_control_look->ComposeIconSize(B_MINI_ICON));
		BBitmap *icon = new BBitmap(rect, B_RGBA32);
		icon_size iconSize = (icon_size)(icon->Bounds().Width()-1);
		status_t status = nodeInfo.GetTrackerIcon(icon, iconSize);
		instance.cache.emplace(mimeType, icon);
		LogTrace("IconCache: GetTrackerIcon returned - %d", status);
		return icon;
	}
	return nullptr;
}

BBitmap*
IconCache::GetIcon(BString path)
{
	BEntry entry(path);
	entry_ref ref;
	entry.GetRef(&ref);
	return IconCache::GetIcon(&ref);
}

void
IconCache::PrintToStream()
{
	
	printf("IconCache %p: cache content\n", &instance);
	for (auto const& x : instance.cache) {
		printf("IconCache: %s\n", x.first.c_str());
	}
	printf("----------------------------\n");
}