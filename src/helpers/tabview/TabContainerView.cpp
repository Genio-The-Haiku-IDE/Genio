/*
 * Copyright (C) 2010 Rene Gollent <rene@gollent.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TabContainerView.h"

#include <Application.h>
#include <AbstractLayoutItem.h>
#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <GroupView.h>
#include <SpaceLayoutItem.h>
#include <Window.h>

#include <cassert>

#include "TabView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tab Manager"

#define TAB_DRAG	'TABD'

#define ALPHA				230
static const float kLeftTabInset = 4;


TabContainerView::TabContainerView(Controller* controller)
	:
	BGroupView(B_HORIZONTAL, 0.0),
	fLastMouseEventTab(NULL),
	fMouseDown(false),
	fClickCount(0),
	fSelectedTab(NULL),
	fController(controller),
	fFirstVisibleTabIndex(0)
{
	SetFlags(Flags() | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	SetViewColor(B_TRANSPARENT_COLOR);
	GroupLayout()->SetInsets(kLeftTabInset, 0, 0, 1);
	GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue(), 0.0f);
}


TabContainerView::~TabContainerView()
{
}


BSize
TabContainerView::MinSize()
{
	// Eventually, we want to be scrolling if the tabs don't fit.
	BSize size(BGroupView::MinSize());
	size.width = 300;
	return size;
}

void
TabContainerView::OnDrop(BMessage* msg)
{
	int32 dragIndex = msg->GetInt32("index", -1);
	if (dragIndex < 0)
		return;
	BPoint drop_point;
	if (msg->FindPoint("_drop_point_", &drop_point) != B_OK)
		return;
	TabView* tab = _TabAt(ConvertFromScreen(drop_point));
	if (!tab)
		return;

	int32 toIndex = IndexOf(tab);
	if (toIndex < 0)
		return;

	//Move
	fController->MoveTabs(dragIndex, toIndex);

	Invalidate();
}
void
TabContainerView::MessageReceived(BMessage* message)
{
	switch (message->what) {
			case TAB_DRAG:
				OnDrop(message);
				break;
			case B_MOUSE_WHEEL_CHANGED:
			{
				// No tabs, exit
				if (fSelectedTab == nullptr)
					break;

				float deltaX = 0.0f;
				float deltaY = 0.0f;
				message->FindFloat("be:wheel_delta_x", &deltaX);
				message->FindFloat("be:wheel_delta_y", &deltaY);

				if (deltaX == 0.0f && deltaY == 0.0f)
					return;

				if (deltaY == 0.0f)
					deltaY = deltaX;

				int32 selection = IndexOf(fSelectedTab);
				int32 numTabs = GroupLayout()->CountItems() - 1;

				if (deltaY > 0  && selection < numTabs - 1) {
					// move to the right tab.
					SelectTab(selection + 1);

				} else if (deltaY < 0 && selection > 0 && numTabs > 1) {
					// move to the left tab.
					SelectTab(selection - 1);

				}
				break;
			}
			case MSG_CLOSE_TAB:
			case MSG_CLOSE_TABS_ALL:
			case MSG_CLOSE_TABS_OTHER:
			{
				TabView* view = NULL;
				if (message->FindPointer("tab_source", (void**)&view) == B_OK) {
					message->AddInt32("tab_index", IndexOf(view));
				}
				fController->HandleTabMenuAction(message);
				break;
			}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
TabContainerView::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	BRect frame(Bounds());

	// Draw empty area before first tab.
	uint32 borders = BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER;
	BRect leftFrame(frame.left, frame.top, kLeftTabInset, frame.bottom);
	be_control_look->DrawInactiveTab(this, leftFrame, updateRect, base, 0,
		borders);

	// Draw all tabs, keeping track of where they end.
	BGroupLayout* layout = GroupLayout();
	int32 count = layout->CountItems() - 1;
	for (int32 i = 0; i < count; i++) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
			layout->ItemAt(i));
		if (!item || !item->IsVisible())
			continue;
		item->Parent()->Draw(updateRect);
		frame.left = item->Frame().right + 1;
	}

	// Draw empty area after last tab.
	be_control_look->DrawInactiveTab(this, frame, updateRect, base, 0, borders);

	_DrawTabIndicator();
}

void
TabContainerView::_DrawTabIndicator()
{
	if (fDropTargetHighlightFrame.IsValid()) {
		rgb_color color = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
		color.alpha = 170;
		SetHighColor(color);
		SetDrawingMode(B_OP_ALPHA);
		FillRect(fDropTargetHighlightFrame);
	}
}


void
TabContainerView::MouseDown(BPoint where)
{
	uint32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons) != B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;
	uint32 clicks;
	if (Window()->CurrentMessage()->FindInt32("clicks", (int32*)&clicks) != B_OK)
		clicks = 1;
	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	if (fLastMouseEventTab)
		fLastMouseEventTab->MouseDown(where, buttons);
	else {
		if ((buttons & B_TERTIARY_MOUSE_BUTTON) != 0) {
#if 0
			// Middle click outside tabs should always open a new tab.
			fController->DoubleClickOutsideTabs();
#endif
		} else if (clicks > 1)
			fClickCount++;
		else
			fClickCount = 1;
	}

	Draggable::OnMouseDown(where);
}


void
TabContainerView::MouseUp(BPoint where)
{
	fMouseDown = false;
	if (fLastMouseEventTab) {
		fLastMouseEventTab->MouseUp(where);
		fClickCount = 0;
	} else if (fClickCount > 1) {
		// NOTE: fClickCount is >= 1 only if the first click was outside
		// any tab. So even if fLastMouseEventTab has been reset to NULL
		// because this tab was removed during mouse down, we wouldn't
		// run the "outside tabs" code below.
#if 0
		fController->DoubleClickOutsideTabs();
#endif
		fClickCount = 0;
	}
	// Always check the tab under the mouse again, since we don't update
	// it with fMouseDown == true.
	_SendFakeMouseMoved();

	Draggable::OnMouseUp(where);
	if (fDropTargetHighlightFrame.IsValid()) {
		Invalidate(fDropTargetHighlightFrame);
		fDropTargetHighlightFrame = BRect();
	}
}

bool
TabContainerView::InitiateDrag(BPoint where)
{
	TabView* tab = _TabAt(where);
	if (tab != nullptr) {
		BMessage message(TAB_DRAG);
		message.AddPointer("container", this);
		message.AddInt32("index", IndexOf(tab));

		BRect updateRect = tab->Frame().OffsetToCopy(BPoint(0,0));

		BBitmap* dragBitmap = new BBitmap(updateRect, B_RGB32, true);
		if (dragBitmap->IsValid()) {
			BView* view = new BView(dragBitmap->Bounds(), "helper", B_FOLLOW_NONE,
				B_WILL_DRAW);
			dragBitmap->AddChild(view);
			dragBitmap->Lock();

			// Drawing (TODO: write a better tab->'Draw'?')
			tab->DrawBackground(view, view->Bounds(), view->Bounds(), true, false, true);
			float spacing = be_control_look->DefaultLabelSpacing();
			updateRect.InsetBy(spacing, spacing / 2);
			tab->DrawContents(view, updateRect, updateRect, true, false, true);
			//

			view->Sync();
			uint8* bits = (uint8*)dragBitmap->Bits();
			int32 height = (int32)dragBitmap->Bounds().Height() + 1;
			int32 width = (int32)dragBitmap->Bounds().Width() + 1;
			int32 bpr = dragBitmap->BytesPerRow();
			for (int32 y = 0; y < height; y++, bits += bpr) {
				uint8* line = bits + 3;
				for (uint8* end = line + 4 * width; line < end; line += 4)
					*line = ALPHA;
			}
			dragBitmap->Unlock();
		} else {
			delete dragBitmap;
			dragBitmap = NULL;
		}

		if (dragBitmap != NULL)
			DragMessage(&message, dragBitmap, B_OP_ALPHA, BPoint(where.x - tab->Frame().left, where.y - tab->Frame().top));
		else
			DragMessage(&message, tab->Frame(), this);

		return true;
	}
	return false;
}


void
TabContainerView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (dragMessage && dragMessage->what == TAB_DRAG) {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW:
			{
				int32 index = dragMessage->GetInt32("index", -1);
				TabView* tab = _TabAt(where);
				if (!tab)
					return;
				int32 newIndex = IndexOf(tab);

				if (index != newIndex) {
					BRect highlightFrame = tab->Frame();

					if (fDropTargetHighlightFrame != highlightFrame) {
						Invalidate(fDropTargetHighlightFrame);
						fDropTargetHighlightFrame = highlightFrame;
						Invalidate(fDropTargetHighlightFrame);
					}
					_MouseMoved(where, transit, dragMessage);
					return;
				}
			}
			break;
		};
	}

	_MouseMoved(where, transit, dragMessage);
	Draggable::OnMouseMoved(where);
	if (fDropTargetHighlightFrame.IsValid()) {
		Invalidate(fDropTargetHighlightFrame);
		fDropTargetHighlightFrame = BRect();
	}
}


void
TabContainerView::DoLayout()
{
	BGroupView::DoLayout();

	_ValidateTabVisibility();
	_SendFakeMouseMoved();
}


void
TabContainerView::AddTab(const char* label, int32 index)
{
	TabView* tab;
	if (fController)
		tab = fController->CreateTabView();
	else
		tab = new TabView();
	tab->SetLabel(label);
	AddTab(tab, index);
}


void
TabContainerView::AddTab(TabView* tab, int32 index)
{
	tab->SetContainerView(this);

	if (index == -1)
		index = GroupLayout()->CountItems() - 1;

	bool hasFrames = fController != NULL && fController->HasFrames();
	bool isFirst = index == 0 && hasFrames;
	bool isLast = index == GroupLayout()->CountItems() - 1 && hasFrames;

	bool isFront = fSelectedTab == NULL;

	tab->Update(isFirst, isLast, isFront);

	GroupLayout()->AddItem(index, tab->LayoutItem());
#if 0
	if (isFront)
		SelectTab(tab);
#endif
	if (isLast) {
		TabLayoutItem* item
			= dynamic_cast<TabLayoutItem*>(GroupLayout()->ItemAt(index - 1));
		if (item)
			item->Parent()->SetIsLast(false);
	}

	SetFirstVisibleTabIndex(MaxFirstVisibleTabIndex());

	_ValidateTabVisibility();
}


TabView*
TabContainerView::RemoveTab(int32 index, int32 currentSelection, bool isLast)
{
	TabLayoutItem* item
		= dynamic_cast<TabLayoutItem*>(GroupLayout()->RemoveItem(index));

	if (!item)
		return NULL;

	BRect dirty(Bounds());
	dirty.left = item->Frame().left;
	TabView* removedTab = item->Parent();
	removedTab->SetContainerView(NULL);

	if (removedTab == fLastMouseEventTab)
		fLastMouseEventTab = NULL;

	// Update tabs after or before the removed tab.
	bool hasFrames = fController != NULL && fController->HasFrames();

#if 0
	item = dynamic_cast<TabLayoutItem*>(GroupLayout()->ItemAt(index));
	if (item) {
		// This tab is behind the removed tab.
		TabView* tab = item->Parent();
		tab->Update(index == 0 && hasFrames,
			index == GroupLayout()->CountItems() - 2 && hasFrames,
			tab == fSelectedTab);
		if (removedTab == fSelectedTab) {
			fSelectedTab = NULL;
			SelectTab(tab);
		} else if (fController && tab == fSelectedTab)
			fController->TabSelected(index);
	} else {
		// The removed tab was the last tab.
		item = dynamic_cast<TabLayoutItem*>(GroupLayout()->ItemAt(index - 1));
		if (item) {
			TabView* tab = item->Parent();
			tab->Update(index == 0 && hasFrames,
				index == GroupLayout()->CountItems() - 2 && hasFrames,
				tab == fSelectedTab);
			if (removedTab == fSelectedTab) {
				fSelectedTab = NULL;
				SelectTab(tab);
			}
		}
	}
#else
	// Index changed if last tab was closed
	if (isLast)
		item = dynamic_cast<TabLayoutItem*>(GroupLayout()->ItemAt(index - 1));
	else
		item = dynamic_cast<TabLayoutItem*>(GroupLayout()->ItemAt(index));

	if (item) {
		TabView* tab = item->Parent();

		tab->Update(index == 0 && hasFrames,
			index == GroupLayout()->CountItems() - 2 && hasFrames,
			tab == fSelectedTab);

		// Manage selection (unselected tab removal too)
		// Index is lower than ex-selection -1 (currentSelection is up-to-date)
		if (index < currentSelection)
			fController->TabSelected(currentSelection);
		// Index is ex-selection - 1
		else if (index == currentSelection)
			fController->TabSelected(index);
		// Index is selected tab
		else if (removedTab == fSelectedTab) {
			fSelectedTab = NULL;
			SelectTab(tab);
		}
		// Index is higher than ex-selection, no reselect needed
	}
#endif

	// To enable reopen if last tab was closed
	if (index == 0 && isLast)
		fSelectedTab = NULL;

	Invalidate(dirty);
	_ValidateTabVisibility();

	return removedTab;
}


TabView*
TabContainerView::TabAt(int32 index) const
{
	TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
		GroupLayout()->ItemAt(index));
	if (item)
		return item->Parent();
	return NULL;
}


int32
TabContainerView::IndexOf(TabView* tab) const
{
	return GroupLayout()->IndexOfItem(tab->LayoutItem());
}


void
TabContainerView::SelectTab(int32 index, BMessage* selInfo)
{
	TabView* tab = NULL;
	TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
		GroupLayout()->ItemAt(index));
	if (item)
		tab = item->Parent();

	SelectTab(tab, selInfo);
}


void
TabContainerView::SelectTab(TabView* tab, BMessage* selInfo)
{
	if (tab == fSelectedTab)
		return;

	if (fSelectedTab)
		fSelectedTab->SetIsFront(false);

	fSelectedTab = tab;

	if (fSelectedTab)
		fSelectedTab->SetIsFront(true);

	if (fController != NULL) {
		int32 index = -1;

		if (fSelectedTab != NULL)
			index = GroupLayout()->IndexOfItem(tab->LayoutItem());

		// scan-build assumes 'Called C++ object pointer is null'
		assert(tab != nullptr);
		if (!tab->LayoutItem()->IsVisible())
			SetFirstVisibleTabIndex(index);

		fController->TabSelected(index, selInfo);
	}
}


void
TabContainerView::SetTabLabel(int32 tabIndex, const char* label)
{
	TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
		GroupLayout()->ItemAt(tabIndex));
	if (item == NULL)
		return;

	item->Parent()->SetLabel(label);
}


void
TabContainerView::SetFirstVisibleTabIndex(int32 index)
{
	if (index < 0)
		index = 0;
	if (index > MaxFirstVisibleTabIndex())
		index = MaxFirstVisibleTabIndex();

	if (fFirstVisibleTabIndex == index)
		return;

	fFirstVisibleTabIndex = index;

	_UpdateTabVisibility();
}


int32
TabContainerView::FirstVisibleTabIndex() const
{
	return fFirstVisibleTabIndex;
}


int32
TabContainerView::MaxFirstVisibleTabIndex() const
{
	float availableWidth = _AvailableWidthForTabs();
	if (availableWidth < 0)
		return 0;
	float visibleTabsWidth = 0;

	BGroupLayout* layout = GroupLayout();
	int32 i = layout->CountItems() - 2;
	for (; i >= 0; i--) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
			layout->ItemAt(i));
		if (item == NULL)
			continue;

		float itemWidth = item->MinSize().width;
		if (availableWidth >= visibleTabsWidth + itemWidth)
			visibleTabsWidth += itemWidth;
		else {
			// The tab before this tab is the last one that can be visible.
			return i + 1;
		}
	}

	return 0;
}


bool
TabContainerView::CanScrollLeft() const
{
	return fFirstVisibleTabIndex < MaxFirstVisibleTabIndex();
}


bool
TabContainerView::CanScrollRight() const
{
	BGroupLayout* layout = GroupLayout();
	int32 count = layout->CountItems() - 1;
	if (count > 0) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
			layout->ItemAt(count - 1));
		return !item->IsVisible();
	}
	return false;
}


// #pragma mark -


TabView*
TabContainerView::_TabAt(const BPoint& where, int32* index) const
{
	BGroupLayout* layout = GroupLayout();
	int32 count = layout->CountItems() - 1;
	if (index)
		*index = -1;
	for (int32 i = 0; i < count; i++) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(layout->ItemAt(i));
		if (item == NULL || !item->IsVisible())
			continue;
		// Account for the fact that the tab frame does not contain the
		// visible bottom border.
		BRect frame = item->Frame();
		frame.bottom++;
		if (frame.Contains(where)) {
			if (index)
				*index = i;
			return item->Parent();
		}
	}
	return NULL;
}


void
TabContainerView::_MouseMoved(BPoint where, uint32 _transit,
	const BMessage* dragMessage)
{
	int32 index;
	TabView* tab = _TabAt(where, &index);
	if (fMouseDown) {
		uint32 transit = tab == fLastMouseEventTab
			? B_INSIDE_VIEW : B_OUTSIDE_VIEW;
		if (fLastMouseEventTab)
			fLastMouseEventTab->MouseMoved(where, transit, dragMessage);
		return;
	}

	if (fLastMouseEventTab != NULL && fLastMouseEventTab == tab)
		fLastMouseEventTab->MouseMoved(where, B_INSIDE_VIEW, dragMessage);
	else {
		if (fLastMouseEventTab)
			fLastMouseEventTab->MouseMoved(where, B_EXITED_VIEW, dragMessage);
		fLastMouseEventTab = tab;
		if (fLastMouseEventTab)
			fLastMouseEventTab->MouseMoved(where, B_ENTERED_VIEW, dragMessage);

		fController->SetToolTip(index);
	}
}


void
TabContainerView::_ValidateTabVisibility()
{
	if (fFirstVisibleTabIndex > MaxFirstVisibleTabIndex())
		SetFirstVisibleTabIndex(MaxFirstVisibleTabIndex());
	else
		_UpdateTabVisibility();
}


void
TabContainerView::_UpdateTabVisibility()
{
	float availableWidth = _AvailableWidthForTabs();
	if (availableWidth < 0)
		return;
	float visibleTabsWidth = 0;

	bool canScrollTabsLeft = fFirstVisibleTabIndex > 0;
	bool canScrollTabsRight = false;

	BGroupLayout* layout = GroupLayout();
	int32 count = layout->CountItems() - 1;
	for (int32 i = 0; i < count; i++) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
			layout->ItemAt(i));
		if (i < fFirstVisibleTabIndex)
			item->SetVisible(false);
		else {
			float itemWidth = item->MinSize().width;
			bool visible = availableWidth >= visibleTabsWidth + itemWidth;
			item->SetVisible(visible && !canScrollTabsRight);
			visibleTabsWidth += itemWidth;
			if (!visible)
				canScrollTabsRight = true;
		}
	}
	fController->UpdateTabScrollability(canScrollTabsLeft, canScrollTabsRight);
}


float
TabContainerView::_AvailableWidthForTabs() const
{
	float width = Bounds().Width() - 10;
		// TODO: Don't really know why -10 is needed above.

	float left;
	float right;
	GroupLayout()->GetInsets(&left, NULL, &right, NULL);
	width -= left + right;

	return width;
}


void
TabContainerView::_SendFakeMouseMoved()
{
	BPoint where;
	uint32 buttons;
	GetMouse(&where, &buttons, false);
	if (Bounds().Contains(where))
		_MouseMoved(where, B_INSIDE_VIEW, NULL);
}
