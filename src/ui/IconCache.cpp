/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "IconCache.h"
#include "Log.h"

IconCache IconCache::instance;

BBitmap*
IconCache::GetIcon(entry_ref *ref)
{	
	BNode node(ref);
	BNodeInfo nodeInfo(&node);
	char mimeType[B_MIME_TYPE_LENGTH];
	nodeInfo.GetType(mimeType);
		
	LogTrace("IconCache: [%s] - [%s]\n", mimeType, ref->name);
	
	auto it = instance.cache.find(mimeType);
	if (it != instance.cache.end()) {
		LogTrace("IconCache: return icon from cache for %s\n",mimeType);
		return it->second;
	} else {
		LogTrace("IconCache: could not find an icon in cache for %s\n",mimeType);
		BRect rect(BPoint(0, 0), be_control_look->ComposeIconSize(B_MINI_ICON));
		BBitmap *icon = new BBitmap(rect, B_RGBA32);
		icon_size iconSize = (icon_size)(icon->Bounds().Width()-1);
		status_t status = nodeInfo.GetTrackerIcon(icon, iconSize);
		instance.cache.emplace(mimeType, icon);
		return icon;
		LogTrace("IconCache: GetTrackerIcon returned - %d\n", status);
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