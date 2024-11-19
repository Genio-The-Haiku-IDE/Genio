/*
 * Copyright 2023-2024, Nexus6 
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
	// TODO: Rework GetIcon() to include icon size as a parameter
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

	auto it = sInstance.fCache.find(mimeTypePtr);
	if (it != sInstance.fCache.end()) {
		LogTrace("IconCache: return icon from cache for %s", mimeTypePtr);
		return it->second;
	} else {
		LogTrace("IconCache: could not find an icon in cache for %s", mimeTypePtr);
		// TODO: we calculate icon size here, but we should pass it as a parameter
		// to GetIcon(), because it's done in StyledItem::DrawIcon, too
		const BSize composedSize = be_control_look->ComposeIconSize(B_MINI_ICON);
		const icon_size iconSize = icon_size(composedSize.IntegerHeight());
		const BRect rect(0, 0, iconSize - 1, iconSize - 1);
		BBitmap *icon = new BBitmap(rect, B_RGBA32);
		status_t status = nodeInfo.GetTrackerIcon(icon, iconSize);
		if (status == B_OK)
			sInstance.fCache.emplace(mimeTypePtr, icon);
		else
			LogTrace("IconCache: GetTrackerIcon returned - %s", ::strerror(status));

		return icon;
	}
	return nullptr;
}


const BBitmap*
IconCache::GetIcon(const BString& path)
{
	const BEntry entry(path);
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
