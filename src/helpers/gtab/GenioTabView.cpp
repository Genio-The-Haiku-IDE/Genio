/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GenioTabView.h"

#include <TabViewPrivate.h>
#include <Layout.h>
#include <CardLayout.h>

class IndexGTab : public GTab  {
public:
	IndexGTab(BView* view, int32 index):GTab(view), fIndex(index){}
	int32	fIndex;
};

BTab*
GenioTabView::_TabFromPoint(BPoint where, int32& index)
{
	index = _TabAt(where);
	if (index < 0)
		return nullptr;
	return TabAt(index);
}


int32
GenioTabView::_TabAt(BPoint where)
{
	for (int32 i = 0; i < CountTabs(); i++) {
		if (TabFrame(i).Contains(where))
			return i;
	}
	return -1;
}

#define TAB_DRAG 'TDRA'
#define ALPHA 230

#include <Bitmap.h>
#include <ControlLook.h>


bool
GenioTabView::InitiateDrag(BPoint where)
{
	uint32 index = _TabAt(where);
	if (index < 0)
		return false;

	BRect frame(TabFrame(index));
	BTab* tab = TabAt(index);
	if (tab != nullptr) {
		BMessage message(TAB_DRAG);

		message.AddPointer("genio_tab_view", this);
		message.AddUInt32("tab_drag_affinity", fTabAffinity);
		message.AddInt32("index", index);

		BRect updateRect = frame.OffsetToCopy(BPoint(0, 0));

		BBitmap* dragBitmap = new BBitmap(updateRect, B_RGB32, true);
		if (dragBitmap->IsValid()) {
			BView* view = new BView(dragBitmap->Bounds(), "helper", B_FOLLOW_NONE, B_WILL_DRAW);
			dragBitmap->AddChild(view);
			dragBitmap->Lock();
			tab->DrawTab(view, updateRect, B_TAB_FRONT, true);

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

		if (dragBitmap != NULL) {
			DragMessage(&message, dragBitmap, B_OP_ALPHA,
				BPoint(where.x - frame.left, where.y - frame.top));
		} else {
			DragMessage(&message, frame, this);
		}

		return true;
	}
	return false;
}


GenioTabView::GenioTabView(const char* name, tab_drag_affinity affinity):
	BTabView(name),
	fTabAffinity(affinity)
{
}


void
GenioTabView::MouseDown(BPoint where)
{
	BTabView::MouseDown(where);
	OnMouseDown(where);
}


void
GenioTabView::MouseUp(BPoint where)
{
	BTabView::MouseUp(where);
	OnMouseUp(where);
	if (fDropTargetHighlightFrame.IsValid()) {
		Invalidate(fDropTargetHighlightFrame);
		fDropTargetHighlightFrame = BRect();
	}
}


void
GenioTabView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	bool sameTabView = true;
	if (dragMessage &&
	    dragMessage->what == TAB_DRAG &&
		_ValidDragAndDrop(dragMessage, &sameTabView)) {
		switch (transit) {
			case B_ENTERED_VIEW:
			case B_INSIDE_VIEW:
			{
				int32 fromIndex = dragMessage->GetInt32("index", -1);
				int32 toIndex = _TabAt(where);
				if (toIndex >= 0) {
					if (fromIndex != toIndex || sameTabView == false) {
						BRect highlightFrame = TabFrame(toIndex);

						if (fDropTargetHighlightFrame != highlightFrame) {
							Invalidate(fDropTargetHighlightFrame);
							fDropTargetHighlightFrame = highlightFrame;
							Invalidate(fDropTargetHighlightFrame);
						}
						return;
					}
				} else {
					float right = 0;
					if (CountTabs() > 0) {
						right = TabFrame(CountTabs()-1).right;
					}
					if (where.x < right)
						return;

					BRect highlightFrame = Bounds();
					highlightFrame.left = right;

					if (fDropTargetHighlightFrame != highlightFrame) {
						Invalidate(fDropTargetHighlightFrame);
						fDropTargetHighlightFrame = highlightFrame;
						Invalidate(fDropTargetHighlightFrame);
					}
					return;
				}
			} break;
			default:
				if (fDropTargetHighlightFrame.IsValid()) {
					Invalidate(fDropTargetHighlightFrame);
					fDropTargetHighlightFrame = BRect();
				}
				break;
		};
	} else {
		BTabView::MouseMoved(where, transit, dragMessage);
		OnMouseMoved(where);
		if (fDropTargetHighlightFrame.IsValid()) {
			Invalidate(fDropTargetHighlightFrame);
			fDropTargetHighlightFrame = BRect();
		}
	}
}


void
GenioTabView::MoveTabs(uint32 from, uint32 to, GenioTabView* fromTabView)
{
	GTabContainer* fromContainer = dynamic_cast<GTabContainer*>(fromTabView->BTabView::ViewForTab(from));
	if (fromContainer == nullptr)
		return;

	BView*	fromView = fromContainer->ContentView();
	if (!fromView)
		return;

	fromView->RemoveSelf();

	BTab* fromTab = fromTabView->TabAt(from);
	BString label = fromTab->Label(); //TODO copy all the props
	fromTabView->RemoveTab(from);
	GTab* indexTab = new IndexGTab(fromView, to);
	indexTab->SetLabel(label.String());
	AddTab(indexTab);
}

void
GenioTabView::AddTab(BView* target, BTab* tab)
{
	BTabView::AddTab(target, tab);
}


void
BTabView::AddTab(BView* target, BTab* tab)
{
	if (tab == NULL)
		tab = new BTab(target);
	else
		tab->SetView(target);

	IndexGTab *gtab = dynamic_cast<IndexGTab *>(tab);
	int32 index = CountTabs();
	if (gtab != nullptr) {
		if (gtab->fIndex < CountTabs())
			index = gtab->fIndex;
	}
//	printf("New Index to: %d\n", index);
	if (fContainerView->GetLayout())
			fContainerView->GetLayout()->AddView(index, target);
	fTabList->AddItem(tab, index);

	BTab::Private(tab).SetTabView(this);

	// When we haven't had a any tabs before, but are already attached to the
	// window, select this one.
	if (Window() != NULL) {
		if (CountTabs() == 1)
			Select(0);
		else {
			Select(index);

		// make the view visible through the layout if there is one
		BCardLayout* layout
			= dynamic_cast<BCardLayout*>(fContainerView->GetLayout());
		if (layout != NULL)
			layout->SetVisibleItem(index);
		}
	}
}

void
GenioTabView::AddTab(BView* target)
{
	AddTab(new GTab(target));
}


BTab*
GenioTabView::TabFromView(BView* view) const
{
	for (int32 i = 0; i < CountTabs(); i++) {
		GTabContainer* fromContainer = dynamic_cast<GTabContainer*>(BTabView::ViewForTab(i));
		if (fromContainer != nullptr && fromContainer->ContentView() == view) {
			return TabAt(i);
		}
	}
	return nullptr;
}


void
GenioTabView::AddTab(GTab* tab)
{
	BTabView::AddTab(tab->View(), tab);
}


void
GenioTabView::_OnDrop(BMessage* msg)
{
	int32 dragIndex = msg->GetInt32("index", -1);
	if (dragIndex < 0)
		return;

	if (!_ValidDragAndDrop(msg))
			return;

	BPoint drop_point;
	if (msg->FindPoint("_drop_point_", &drop_point) != B_OK)
		return;

	int32 toIndex = _TabAt(ConvertFromScreen(drop_point));
	if (CountTabs() == 0)
		toIndex = 0;

	if (toIndex < 0) {
		if (drop_point.x < TabFrame(CountTabs()-1).right)
			return;
		toIndex = CountTabs();
	}

	GenioTabView* fromTabView = (GenioTabView*)msg->GetPointer("genio_tab_view", this);

	MoveTabs(dragIndex, toIndex, fromTabView);
	Invalidate();
	if (fromTabView != this)
		fromTabView->Invalidate();
}


void
GenioTabView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case TAB_DRAG:
		{
			_OnDrop(msg);
		} break;
		default:
			BTabView::MessageReceived(msg);
			break;
	};
}
/*
void
GenioTabView::AddTab(BView* target, BTab* tab)
{
	BTabView::AddTab(target, tab);
}
*/
BRect
GenioTabView::DrawTabs()
{
	BRect rect = BTabView::DrawTabs();
	_DrawTabIndicator();
	return rect;
}


void
GenioTabView::_DrawTabIndicator()
{
	if (fDropTargetHighlightFrame.IsValid()) {
		rgb_color color = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
		color.alpha = 170;
		SetHighColor(color);
		SetDrawingMode(B_OP_ALPHA);
		FillRect(fDropTargetHighlightFrame);
	}
}

bool
GenioTabView::_ValidDragAndDrop(const BMessage* msg, bool* sameTabView)
{
	if (sameTabView)
		*sameTabView = (msg->GetPointer("genio_tab_view", nullptr) == this);

	if (fTabAffinity == 0 && msg->GetPointer("genio_tab_view", nullptr) != this)
		return false;

	if (msg->GetUInt32("tab_drag_affinity", 0) != fTabAffinity)
		return false;

	return true;
}