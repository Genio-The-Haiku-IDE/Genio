/*
 * Copyright 2025, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <OutlineListView.h>


// Implements a listview which can show a popup menu on right click
// inherited class should reimplement ShowPopUpMenu()
class GOutlineListView: public BOutlineListView {
public:
	GOutlineListView(const char* name, list_view_type type = B_SINGLE_SELECTION_LIST,
					uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);

	void MouseMoved(BPoint point, uint32 transit, const BMessage* message) override;
	void MouseDown(BPoint where) override;
	void MouseUp(BPoint where) override;

protected:
	virtual void ShowPopupMenu(BPoint where);
};
