/*
 * Copyright 2025, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GOutlineListView.h"

#include <Looper.h>

#include "StyledItem.h"


GOutlineListView::GOutlineListView(const char* name, list_view_type type, uint32 flags)
	:
	BOutlineListView(name, type, flags)
{
}


void
GOutlineListView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (transit == B_ENTERED_VIEW || transit == B_INSIDE_VIEW) {
		const auto index = IndexOf(point);
		if (index >= 0) {
			BString toolTipText;
			StyledItem *item = dynamic_cast<StyledItem*>(ItemAt(index));
			if (item != nullptr && item->HasToolTip()) {
				toolTipText = item->GetToolTipText();
			}
			SetToolTip(toolTipText);
		}
	}
	// We don't call inherited MouseMoved(): it's not an error
}


void
GOutlineListView::MouseDown(BPoint where)
{
	int32 buttons = -1;
	const BMessage* message = Looper()->CurrentMessage();
	if (message != NULL)
		message->FindInt32("buttons", &buttons);

	if (buttons == B_MOUSE_BUTTON(1)) {
		return BOutlineListView::MouseDown(where);
	} else  if ( buttons == B_MOUSE_BUTTON(2)) {
		int32 index = IndexOf(where);
		if (index >= 0) {
			Select(index);
			ShowPopupMenu(where);
		}
	}
}


void
GOutlineListView::MouseUp(BPoint where)
{
	BOutlineListView::MouseUp(where);
}


/* virtual */
void
GOutlineListView::ShowPopupMenu(BPoint where)
{
}
