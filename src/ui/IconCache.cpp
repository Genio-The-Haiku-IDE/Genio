/*
 * Copyright 2023, Nexus6 
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
IconCache::GetIcon(const entry_ref *ref)
{
	BNode node(ref);
	BNodeInfo nodeInfo(&node);
	char mimeType[B_MIME_TYPE_LENGTH];
	mimeType[0] = '\0';
	if (nodeInfo.GetType(mimeType) != B_OK) {
		LogDebug("Invalid mimeType for file [%s]", ref->name);
		if (node.IsDirectory())
			strncpy(mimeType, B_DIRECTORY_MIME_TYPE, B_MIME_TYPE_LENGTH - 1);
		else
			strncpy(mimeType, B_FILE_MIME_TYPE, B_MIME_TYPE_LENGTH - 1);
	}

	LogTrace("IconCache: [%s] - [%s]", mimeType, ref->name);

	auto it = sInstance.fCache.find(mimeType);
	if (it != sInstance.fCache.end()) {
		LogTrace("IconCache: return icon from cache for %s",mimeType);
		return it->second;
	} else {
		LogTrace("IconCache: could not find an icon in cache for %s",mimeType);
		BSize composedSize = be_control_look->ComposeIconSize(B_MINI_ICON);
		BRect rect(0, 0, composedSize.IntegerWidth() - 1, composedSize.IntegerHeight() - 1);
		BBitmap *icon = new BBitmap(rect, B_RGBA32);
		icon_size iconSize = (icon_size)(icon->Bounds().IntegerWidth() - 1);
		status_t status = nodeInfo.GetTrackerIcon(icon, (icon_size)iconSize);
		sInstance.fCache.emplace(mimeType, icon);
		LogTrace("IconCache: GetTrackerIcon returned - %s", ::strerror(status));
		return icon;
	}
	return nullptr;
}


const BBitmap*
IconCache::GetIcon(const BString& path)
{
	BEntry entry(path);
	entry_ref ref;
	entry.GetRef(&ref);
	return IconCache::GetIcon(&ref);
}


void
IconCache::PrintToStream()
{
	printf("IconCache %p: cache content\n", &sInstance);
	for (auto const& x : sInstance.fCache) {
		printf("IconCache: %s\n", x.first.c_str());
	}
	printf("----------------------------\n");
}
