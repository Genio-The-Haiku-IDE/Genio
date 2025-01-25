/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "TabsContainer.h"

#include <cassert>

#include "GTab.h"
#include "GTabView.h"

#define FILLER_WEIGHT 0.2

TabsContainer::TabsContainer(GTabView* tabView,
							 tab_affinity affinity,
							 BMessage* message)
	:
	BGroupView(B_HORIZONTAL, 0.0f),
	BInvoker(message, nullptr, nullptr),
	fSelectedTab(nullptr),
	fGTabView(tabView),
	fTabShift(0),
	fAffinity(affinity)
{
	SetFlags(Flags()|B_FRAME_EVENTS);
	GroupLayout()->AddView(0, new Filler(this));
	GroupLayout()->SetItemWeight(0, FILLER_WEIGHT);
	SetExplicitMinSize(BSize(50, TabViewTools::DefaultTabHeigh()));
	SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
}


void
TabsContainer::AddTab(GTab* tab, int32 index)
{
	if (index == -1 || index > CountTabs())
		index = CountTabs();

	BLayoutItem* item = GroupLayout()->AddView(index, tab);
	tab->SetLayoutItem(item);

	tab->SetContainer(this);

	if (CountTabs() == 1) {
		SelectTab(tab);
	}

	ShiftTabs(0);
}


int32
TabsContainer::CountTabs() const
{
	return GroupLayout()->CountItems() - 1; //exclude the Filler.
}


GTab*
TabsContainer::TabAt(int32 index) const
{
	if (index < 0 || index >= CountTabs())
		return nullptr;

	return dynamic_cast<GTab*>(GroupLayout()->ItemAt(index)->View());
}


int32
TabsContainer::IndexOfTab(GTab* tab) const
{
	if (tab == nullptr)
		return -1;

	return GroupLayout()->IndexOfItem(tab->LayoutItem());
}


GTab*
TabsContainer::RemoveTab(GTab* tab)
{
	int32 selectedIndex = IndexOfTab(tab);
	tab->LayoutItem()->RemoveSelf();
	tab->RemoveSelf();

	if (CountTabs() == 0) {
		SelectTab(nullptr);
	} else  if (selectedIndex >= CountTabs()) {
		SelectTab(TabAt(CountTabs() - 1));
	} else {
		SelectTab(TabAt(selectedIndex));
	}

	delete tab->LayoutItem();
	tab->SetLayoutItem(nullptr);

	// fix tab visibility
	// TODO: this could be further improved by shifting according to the free
	// available space.
	int32 shift = 0;
	if (fTabShift > 0 && fTabShift >= CountTabs()) {
		shift -= 1;
	}
	ShiftTabs(shift);

	return tab;
}


GTab*
TabsContainer::SelectedTab() const
{
	return fSelectedTab;
}


void
TabsContainer::SelectTab(GTab* tab, bool invoke)
{
	if (tab != fSelectedTab) {
		if (fSelectedTab)
			fSelectedTab->SetIsFront(false);

		fSelectedTab = tab;

		if (fSelectedTab)
			fSelectedTab->SetIsFront(true);

		int32 index = IndexOfTab(fSelectedTab);
		if (invoke && Message() != nullptr && Target() != nullptr) {
			BMessage msg = *Message();
			msg.AddPointer("tab", fSelectedTab);
			msg.AddInt32("index", IndexOfTab(fSelectedTab));
			Invoke(&msg);
		}

		if (fTabShift >= index) {
			ShiftTabs(index - fTabShift);
		} else {
			// let's ensure at least the tab's "middle point"
			// is visible.
			float middle = fSelectedTab->Frame().right - (fSelectedTab->Frame().Width() / 2.0f);
			if (middle > Bounds().right) {
				int32 shift = 0;
				for (int32 i = fTabShift; i < CountTabs(); i++) {
					GTab* nextTab = TabAt(i);
					middle -= nextTab->Bounds().Width();
					shift++;
					if (middle < Bounds().right) {
						break;
					}
				}
				ShiftTabs(shift);
			}
		}
	}
}



void
TabsContainer::ShiftTabs(int32 delta)
{
	int32 newShift = fTabShift + delta;
	if (newShift < 0)
		newShift = 0;

	int32 max = std::max(newShift, fTabShift);
	for (int32 i = 0; i < max; i++) {
		GTab* tab = TabAt(i);
		if (i < newShift) {
			if (tab->IsHidden() == false)
				tab->Hide();
		} else {
			if (tab->IsHidden() == true)
				tab->Show();
		}
	}
	fTabShift = newShift;
	_UpdateScrolls();
}


void
TabsContainer::MouseDownOnTab(GTab* tab, BPoint where, const int32 buttons)
{
	if(buttons & B_PRIMARY_MOUSE_BUTTON) {
		SelectTab(tab);
	} else if (buttons & B_TERTIARY_MOUSE_BUTTON) {
		// Nothing
	}
}



void
TabsContainer::FrameResized(float w, float h)
{
	// Auto-scroll:
	if (fTabShift > 0) {
		int32 tox = 0;
		GTab* last = TabAt(CountTabs()-1);
		float right =  last->Frame().right;
		for (int32 i = fTabShift - 1; i >= 0; i--){
			GTab* tab = TabAt(i);
			right =  right + tab->Frame().Width();
			if (right < w)
				tox--;
			else
				break;
		}
		if (tox != 0)
			ShiftTabs(tox);
	}
	// end
	_UpdateScrolls();
	BGroupView::FrameResized(w,h);
}


void
TabsContainer::OnDropTab(GTab* toTab, BMessage* message)
{
	GTab* fromTab = (GTab*)message->GetPointer("tab", nullptr);
	TabsContainer*	fromContainer = fromTab->Container();
	if (fromTab == nullptr || fromContainer == nullptr || toTab == fromTab)
		return;

	fGTabView->MoveTabs(fromTab, toTab, fromContainer);
}


void
TabsContainer::_PrintToStream()
{
	for (int32 i = 0; i < GroupLayout()->CountItems(); i++) {
		printf("%" B_PRIi32 ") %s\n", i, GroupLayout()->ItemAt(i)->View()->Name());
	}
}


void
TabsContainer::_UpdateScrolls()
{
	if (CountTabs() > 0) {
		GroupLayout()->Relayout(true);
		GTab* last = TabAt(CountTabs() - 1);
		if (fGTabView != nullptr && last != nullptr)
			fGTabView->UpdateScrollButtons(fTabShift != 0, last->Frame().right > Bounds().right);
	} else {
			fGTabView->UpdateScrollButtons(false, false);
	}
}
