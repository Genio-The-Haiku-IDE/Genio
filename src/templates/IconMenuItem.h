/*
 * Copyright 2023 Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 * Parts are taken from the IconMenuItem class from Haiku (Tracker) under the
 * Open Tracker Licence
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */

#ifndef ICON_MENU_ITEM_H
#define ICON_MENU_ITEM_H


#include <MenuItem.h>
#include <NodeInfo.h>

class BBitmap;
class BNodeInfo;

const bigtime_t kSynchMenuInvokeTimeout = 5000000;
const color_space kDefaultIconDepth = B_RGBA32;

class IconMenuItem : public BMenuItem {
	public:
		IconMenuItem(const char* label, BMessage* message, BBitmap* icon,
			icon_size which = B_MINI_ICON);
		IconMenuItem(const char* label, BMessage* message,
			const char* iconType, icon_size which = B_MINI_ICON);
		IconMenuItem(const char* label, BMessage* message,
			const BNodeInfo* nodeInfo, icon_size which);
		IconMenuItem(BMenu*, BMessage*, const char* iconType,
			icon_size which = B_MINI_ICON);
		IconMenuItem(BMessage* data);
		virtual ~IconMenuItem();

		static BArchivable* Instantiate(BMessage* data);
		virtual status_t Archive(BMessage* data, bool deep = true) const;

		virtual void GetContentSize(float* width, float* height);
		virtual void DrawContent();
		virtual void SetMarked(bool mark);

private:
		virtual void SetIcon(BBitmap* icon);

	private:
		BBitmap* fDeviceIcon;
		float fHeightDelta;
		icon_size fWhich;

		typedef BMenuItem _inherited;
};

#endif	// ICON_MENU_ITEM_H
