/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 * Parts are taken from the IconMenuItem class from Haiku (Tracker) under the
 * Open Tracker Licence
 * Copyright (c) 1991-2000, Be Incorporated. All rights reserved.
 */

#include "IconMenuItem.h"

#include <Bitmap.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Menu.h>
#include <MenuField.h>


#include "IconCache.h"

IconMenuItem::IconMenuItem(const char* label, BMessage* message, BBitmap* icon,
	icon_size which)
	:
	BMenuItem(label, message),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(which)
{
	SetIcon(icon);

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(const char* label, BMessage* message,
	const BNodeInfo* nodeInfo, icon_size which)
	:
	BMenuItem(label, message),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(which)
{
	if (nodeInfo != NULL) {
		fDeviceIcon = new BBitmap(BRect(BPoint(0, 0),
			be_control_look->ComposeIconSize(which)), kDefaultIconDepth);
		if (nodeInfo->GetTrackerIcon(fDeviceIcon, (icon_size)-1) != B_OK) {
			delete fDeviceIcon;
			fDeviceIcon = NULL;
		}
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(const char* label, BMessage* message,
	const char* iconType, icon_size which)
	:
	BMenuItem(label, message),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(which)
{
	BMimeType mime(iconType);
	fDeviceIcon = new BBitmap(BRect(BPoint(0, 0), be_control_look->ComposeIconSize(which)),
		kDefaultIconDepth);

	if (mime.GetIcon(fDeviceIcon, which) != B_OK) {
		BMimeType super;
		mime.GetSupertype(&super);
		if (super.GetIcon(fDeviceIcon, which) != B_OK) {
			delete fDeviceIcon;
			fDeviceIcon = NULL;
		}
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(BMenu* submenu, BMessage* message,
							const char* iconType, icon_size which)
	:
	BMenuItem(submenu, message),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(which)
{
	BMimeType mime(iconType);
	fDeviceIcon = new BBitmap(BRect(BPoint(0, 0), be_control_look->ComposeIconSize(which)),
		kDefaultIconDepth);

	if (mime.GetIcon(fDeviceIcon, which) != B_OK) {
		BMimeType super;
		mime.GetSupertype(&super);
		if (super.GetIcon(fDeviceIcon, which) != B_OK) {
			delete fDeviceIcon;
			fDeviceIcon = NULL;
		}
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


IconMenuItem::IconMenuItem(BMenu* menu, BMessage* message, BBitmap* icon, icon_size which)
			:
			BMenuItem(menu, message),
			fDeviceIcon(NULL),
			fHeightDelta(0),
			fWhich(which)
{
	SetIcon(icon);

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}

IconMenuItem::IconMenuItem(BMessage* data)
	:
	BMenuItem(data),
	fDeviceIcon(NULL),
	fHeightDelta(0),
	fWhich(B_MINI_ICON)
{
	if (data != NULL) {
		fWhich = (icon_size)data->GetInt32("_which", B_MINI_ICON);

		fDeviceIcon = new BBitmap(BRect(BPoint(0, 0), be_control_look->ComposeIconSize(fWhich)),
			kDefaultIconDepth);

		if (data->HasData("_deviceIconBits", B_RAW_TYPE)) {
			ssize_t numBytes;
			const void* bits;
			if (data->FindData("_deviceIconBits", B_RAW_TYPE, &bits, &numBytes)
					== B_OK) {
				fDeviceIcon->SetBits(bits, numBytes, (int32)0,
					kDefaultIconDepth);
			}
		}
	}

	// IconMenuItem is used in synchronously invoked menus, make sure
	// we invoke with a timeout
	SetTimeout(kSynchMenuInvokeTimeout);
}


BArchivable*
IconMenuItem::Instantiate(BMessage* data)
{
	//if (validate_instantiation(data, "IconMenuItem"))
		return new IconMenuItem(data);

	return NULL;
}


status_t
IconMenuItem::Archive(BMessage* data, bool deep) const
{
	status_t result = _inherited::Archive(data, deep);

	if (result == B_OK)
		result = data->AddInt32("_which", (int32)fWhich);

	if (result == B_OK && fDeviceIcon != NULL) {
		result = data->AddData("_deviceIconBits", B_RAW_TYPE,
			fDeviceIcon->Bits(), fDeviceIcon->BitsLength());
	}

	return result;
}


IconMenuItem::~IconMenuItem()
{
	delete fDeviceIcon;
}


void
IconMenuItem::GetContentSize(float* width, float* height)
{
	_inherited::GetContentSize(width, height);

	int32 iconHeight = fWhich;
	if (fDeviceIcon != NULL)
		iconHeight = fDeviceIcon->Bounds().IntegerHeight() + 1;

	fHeightDelta = iconHeight - *height;
	if (*height < iconHeight)
		*height = iconHeight;

	*width += 20;
}


void
IconMenuItem::DrawContent()
{
	BPoint drawPoint(ContentLocation());
	if (fDeviceIcon != NULL)
		drawPoint.x += (fDeviceIcon->Bounds().Width() + 1) + 4.0f;

	if (fHeightDelta > 0)
		drawPoint.y += ceilf(fHeightDelta / 2);

	Menu()->MovePenTo(drawPoint);
	_inherited::DrawContent();

	Menu()->PushState();

	BPoint where(ContentLocation());
	float deltaHeight = fHeightDelta < 0 ? -fHeightDelta : 0;
	where.y += ceilf(deltaHeight / 2);

	if (fDeviceIcon != NULL) {
		if (IsEnabled())
			Menu()->SetDrawingMode(B_OP_ALPHA);
		else {
			Menu()->SetDrawingMode(B_OP_ALPHA);
			Menu()->SetHighColor(0, 0, 0, 64);
			Menu()->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		}
		Menu()->DrawBitmapAsync(fDeviceIcon, where);
	}

	Menu()->PopState();
}


void
IconMenuItem::SetMarked(bool mark)
{
	_inherited::SetMarked(mark);

	if (!mark)
		return;

	// we are marking the item

	BMenu* menu = Menu();
	if (menu == NULL)
		return;

	// we have a parent menu

	BMenu* _menu = menu;
	while ((_menu = _menu->Supermenu()) != NULL)
		menu = _menu;

	// went up the hierarchy to found the topmost menu

	if (menu == NULL || menu->Parent() == NULL)
		return;

	// our topmost menu has a parent

	if (dynamic_cast<BMenuField*>(menu->Parent()) == NULL)
		return;

	// our topmost menu's parent is a BMenuField

	BMenuItem* topLevelItem = menu->ItemAt((int32)0);

	if (topLevelItem == NULL)
		return;

	// our topmost menu has a menu item

	IconMenuItem* topLevelIconMenuItem
		= dynamic_cast<IconMenuItem*>(topLevelItem);
	if (topLevelIconMenuItem == NULL)
		return;

	// our topmost menu's item is an IconMenuItem

	// update the icon
	topLevelIconMenuItem->SetIcon(fDeviceIcon);
	menu->Invalidate();
}


void
IconMenuItem::SetIcon(BBitmap* icon)
{
	if (icon != NULL) {
		if (fDeviceIcon != NULL)
			delete fDeviceIcon;

		fDeviceIcon = new BBitmap(BRect(BPoint(0, 0),
			be_control_look->ComposeIconSize(fWhich)), icon->ColorSpace());
		fDeviceIcon->ImportBits(icon);
	} else {
		delete fDeviceIcon;
		fDeviceIcon = NULL;
	}
}
