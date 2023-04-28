/*
 * Copyright 2001-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Axel Dörfler, axeld@pinc-software.de
 *		Rene Gollent (rene@gollent.com)
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		John Scipione, jscipione@gmail.com
 */


//! BOutlineListView represents a "nestable" list view.


#include <OutlineListView.h>

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>

#include <ControlLook.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


typedef int (*compare_func)(const BListItem* a, const BListItem* b);


struct ListItemComparator {
	ListItemComparator(compare_func compareFunc)
		:
		fCompareFunc(compareFunc)
	{
	}

	bool operator()(const BListItem* a, const BListItem* b) const
	{
		return fCompareFunc(a, b) < 0;
	}

private:
	compare_func	fCompareFunc;
};


static void
_GetSubItems(BList& sourceList, BList& destList, BListItem* parent, int32 start)
{
	for (int32 i = start; i < sourceList.CountItems(); i++) {
		BListItem* item = (BListItem*)sourceList.ItemAt(i);
		if (item->OutlineLevel() <= parent->OutlineLevel())
			break;
		destList.AddItem(item);
	}
}


static void
_DoSwap(BList& list, int32 firstIndex, int32 secondIndex, BList* firstItems,
	BList* secondItems)
{
	BListItem* item = (BListItem*)list.ItemAt(firstIndex);
	list.SwapItems(firstIndex, secondIndex);
	list.RemoveItems(secondIndex + 1, secondItems->CountItems());
	list.RemoveItems(firstIndex + 1, firstItems->CountItems());
	list.AddList(secondItems, firstIndex + 1);
	int32 newIndex = list.IndexOf(item);
	if (newIndex + 1 < list.CountItems())
		list.AddList(firstItems, newIndex + 1);
	else
		list.AddList(firstItems);
}


//	#pragma mark - BOutlineListView


BOutlineListView::BOutlineListView(BRect frame, const char* name,
	list_view_type type, uint32 resizingMode, uint32 flags)
	:
	BListView(frame, name, type, resizingMode, flags)
{
}


BOutlineListView::BOutlineListView(const char* name, list_view_type type,
	uint32 flags)
	:
	BListView(name, type, flags)
{
}


BOutlineListView::BOutlineListView(BMessage* archive)
	:
	BListView(archive)
{
	int32 i = 0;
	BMessage subData;
	while (archive->FindMessage("_l_full_items", i++, &subData) == B_OK) {
		BArchivable* object = instantiate_object(&subData);
		if (!object)
			continue;

		BListItem* item = dynamic_cast<BListItem*>(object);
		if (item)
			AddItem(item);
	}
}


BOutlineListView::~BOutlineListView()
{
	fFullList.MakeEmpty();
}


BArchivable*
BOutlineListView::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BOutlineListView"))
		return new BOutlineListView(archive);

	return NULL;
}


status_t
BOutlineListView::Archive(BMessage* archive, bool deep) const
{
	// Note: We can't call the BListView Archive function here, as we are also
	// interested in subitems BOutlineListView can have. They are even stored
	// with a different field name (_l_full_items vs. _l_items).

	status_t status = BView::Archive(archive, deep);
	if (status != B_OK)
		return status;

	status = archive->AddInt32("_lv_type", fListType);
	if (status == B_OK && deep) {
		int32 i = 0;
		BListItem* item = NULL;
		while ((item = static_cast<BListItem*>(fFullList.ItemAt(i++)))) {
			BMessage subData;
			status = item->Archive(&subData, true);
			if (status >= B_OK)
				status = archive->AddMessage("_l_full_items", &subData);

			if (status < B_OK)
				break;
		}
	}

	if (status >= B_OK && InvocationMessage() != NULL)
		status = archive->AddMessage("_msg", InvocationMessage());

	if (status == B_OK && fSelectMessage != NULL)
		status = archive->AddMessage("_2nd_msg", fSelectMessage);

	return status;
}


void
BOutlineListView::MouseDown(BPoint where)
{
	MakeFocus();

	int32 index = IndexOf(where);

	if (index != -1) {
		BListItem* item = ItemAt(index);

		if (item->fHasSubitems
			&& LatchRect(ItemFrame(index), item->fLevel).Contains(where)) {
			if (item->IsExpanded())
				Collapse(item);
			else
				Expand(item);
		} else
			BListView::MouseDown(where);
	}
}


void
BOutlineListView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		int32 currentSel = CurrentSelection();
		switch (bytes[0]) {
			case B_RIGHT_ARROW:
			{
				BListItem* item = ItemAt(currentSel);
				if (item && item->fHasSubitems) {
					if (!item->IsExpanded())
						Expand(item);
					else {
						Select(currentSel + 1);
						ScrollToSelection();
					}
				}
				return;
			}

			case B_LEFT_ARROW:
			{
				BListItem* item = ItemAt(currentSel);
				if (item) {
					if (item->fHasSubitems && item->IsExpanded())
						Collapse(item);
					else {
						item = Superitem(item);
						if (item) {
							Select(IndexOf(item));
							ScrollToSelection();
						}
					}
				}
				return;
			}
		}
	}

	BListView::KeyDown(bytes, numBytes);
}


void
BOutlineListView::FrameMoved(BPoint newPosition)
{
	BListView::FrameMoved(newPosition);
}


void
BOutlineListView::FrameResized(float newWidth, float newHeight)
{
	BListView::FrameResized(newWidth, newHeight);
}


void
BOutlineListView::MouseUp(BPoint where)
{
	BListView::MouseUp(where);
}


bool
BOutlineListView::AddUnder(BListItem* item, BListItem* superItem)
{
	if (superItem == NULL)
		return AddItem(item);

	fFullList.AddItem(item, FullListIndexOf(superItem) + 1);

	item->fLevel = superItem->OutlineLevel() + 1;
	superItem->fHasSubitems = true;

	if (superItem->IsItemVisible() && superItem->IsExpanded()) {
		item->SetItemVisible(true);

		int32 index = BListView::IndexOf(superItem);

		BListView::AddItem(item, index + 1);
		Invalidate(LatchRect(ItemFrame(index), superItem->OutlineLevel()));
	} else
		item->SetItemVisible(false);

	return true;
}


bool
BOutlineListView::AddItem(BListItem* item)
{
	return AddItem(item, FullListCountItems());
}


bool
BOutlineListView::AddItem(BListItem* item, int32 fullListIndex)
{
	if (fullListIndex < 0)
		fullListIndex = 0;
	else if (fullListIndex > FullListCountItems())
		fullListIndex = FullListCountItems();

	if (!fFullList.AddItem(item, fullListIndex))
		return false;

	// Check if this item is visible, and if it is, add it to the
	// other list, too

	if (item->fLevel > 0) {
		BListItem* super = _SuperitemForIndex(fullListIndex, item->fLevel);
		if (super == NULL)
			return true;

		bool hadSubitems = super->fHasSubitems;
		super->fHasSubitems = true;

		if (!super->IsItemVisible() || !super->IsExpanded()) {
			item->SetItemVisible(false);
			return true;
		}

		if (!hadSubitems) {
			Invalidate(LatchRect(ItemFrame(IndexOf(super)),
				super->OutlineLevel()));
		}
	}

	int32 listIndex = _FindPreviousVisibleIndex(fullListIndex);

	if (!BListView::AddItem(item, IndexOf(FullListItemAt(listIndex)) + 1)) {
		// adding didn't work out, we need to remove it from the main list again
		fFullList.RemoveItem(fullListIndex);
		return false;
	}

	return true;
}


bool
BOutlineListView::AddList(BList* newItems)
{
	return AddList(newItems, FullListCountItems());
}


bool
BOutlineListView::AddList(BList* newItems, int32 fullListIndex)
{
	if ((newItems == NULL) || (newItems->CountItems() == 0))
		return false;

	for (int32 i = 0; i < newItems->CountItems(); i++)
		AddItem((BListItem*)newItems->ItemAt(i), fullListIndex + i);

	return true;
}


bool
BOutlineListView::RemoveItem(BListItem* item)
{
	return _RemoveItem(item, FullListIndexOf(item)) != NULL;
}


BListItem*
BOutlineListView::RemoveItem(int32 fullListIndex)
{
	return _RemoveItem(FullListItemAt(fullListIndex), fullListIndex);
}


bool
BOutlineListView::RemoveItems(int32 fullListIndex, int32 count)
{
	if (fullListIndex >= FullListCountItems())
		fullListIndex = -1;
	if (fullListIndex < 0)
		return false;

	// TODO: very bad for performance!!
	while (count--)
		BOutlineListView::RemoveItem(fullListIndex);

	return true;
}


BListItem*
BOutlineListView::FullListItemAt(int32 fullListIndex) const
{
	return (BListItem*)fFullList.ItemAt(fullListIndex);
}


int32
BOutlineListView::FullListIndexOf(BPoint where) const
{
	int32 index = BListView::IndexOf(where);

	if (index > 0)
		index = _FullListIndex(index);

	return index;
}


int32
BOutlineListView::FullListIndexOf(BListItem* item) const
{
	return fFullList.IndexOf(item);
}


BListItem*
BOutlineListView::FullListFirstItem() const
{
	return (BListItem*)fFullList.FirstItem();
}


BListItem*
BOutlineListView::FullListLastItem() const
{
	return (BListItem*)fFullList.LastItem();
}


bool
BOutlineListView::FullListHasItem(BListItem* item) const
{
	return fFullList.HasItem(item);
}


int32
BOutlineListView::FullListCountItems() const
{
	return fFullList.CountItems();
}


int32
BOutlineListView::FullListCurrentSelection(int32 index) const
{
	int32 i = BListView::CurrentSelection(index);

	BListItem* item = BListView::ItemAt(i);
	if (item)
		return fFullList.IndexOf(item);

	return -1;
}


void
BOutlineListView::MakeEmpty()
{
	fFullList.MakeEmpty();
	BListView::MakeEmpty();
}


bool
BOutlineListView::FullListIsEmpty() const
{
	return fFullList.IsEmpty();
}


void
BOutlineListView::FullListDoForEach(bool(*func)(BListItem* item))
{
	fFullList.DoForEach(reinterpret_cast<bool (*)(void*)>(func));
}


void
BOutlineListView::FullListDoForEach(bool (*func)(BListItem* item, void* arg),
	void* arg)
{
	fFullList.DoForEach(reinterpret_cast<bool (*)(void*, void*)>(func), arg);
}


BListItem*
BOutlineListView::Superitem(const BListItem* item)
{
	int32 index = FullListIndexOf((BListItem*)item);
	if (index == -1)
		return NULL;

	return _SuperitemForIndex(index, item->OutlineLevel());
}


void
BOutlineListView::Expand(BListItem* item)
{
	ExpandOrCollapse(item, true);
}


void
BOutlineListView::Collapse(BListItem* item)
{
	ExpandOrCollapse(item, false);
}


bool
BOutlineListView::IsExpanded(int32 fullListIndex)
{
	BListItem* item = FullListItemAt(fullListIndex);
	if (!item)
		return false;

	return item->IsExpanded();
}


BHandler*
BOutlineListView::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return BListView::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
BOutlineListView::GetSupportedSuites(BMessage* data)
{
	return BListView::GetSupportedSuites(data);
}


status_t
BOutlineListView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BOutlineListView::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BOutlineListView::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BOutlineListView::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BOutlineListView::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BOutlineListView::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BOutlineListView::GetHeightForWidth(data->width, &data->min,
				&data->max, &data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BOutlineListView::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BOutlineListView::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BOutlineListView::DoLayout();
			return B_OK;
		}
	}

	return BListView::Perform(code, _data);
}


void
BOutlineListView::ResizeToPreferred()
{
	BListView::ResizeToPreferred();
}


void
BOutlineListView::GetPreferredSize(float* _width, float* _height)
{
	int32 count = CountItems();

	if (count > 0) {
		float maxWidth = 0.0;
		for (int32 i = 0; i < count; i++) {
			// The item itself does not take his OutlineLevel into account, so
			// we must make up for that. Also add space for the latch.
			float itemWidth = ItemAt(i)->Width() + be_plain_font->Size()
				+ (ItemAt(i)->OutlineLevel() + 1)
					* be_control_look->DefaultItemSpacing();
			if (itemWidth > maxWidth)
				maxWidth = itemWidth;
		}

		if (_width != NULL)
			*_width = maxWidth;
		if (_height != NULL)
			*_height = ItemAt(count - 1)->Bottom();
	} else
		BView::GetPreferredSize(_width, _height);
}


void
BOutlineListView::MakeFocus(bool state)
{
	BListView::MakeFocus(state);
}


void
BOutlineListView::AllAttached()
{
	BListView::AllAttached();
}


void
BOutlineListView::AllDetached()
{
	BListView::AllDetached();
}


void
BOutlineListView::DetachedFromWindow()
{
	BListView::DetachedFromWindow();
}


void
BOutlineListView::FullListSortItems(int (*compareFunc)(const BListItem* a,
	const BListItem* b))
{
	SortItemsUnder(NULL, false, compareFunc);
}


void
BOutlineListView::SortItemsUnder(BListItem* superItem, bool oneLevelOnly,
	int (*compareFunc)(const BListItem* a, const BListItem* b))
{
	// This method is quite complicated: basically, it creates a real tree
	// from the items of the full list, sorts them as needed, and then
	// populates the entries back into the full and display lists

	int32 firstIndex = FullListIndexOf(superItem) + 1;
	int32 lastIndex = firstIndex;
	BList* tree = _BuildTree(superItem, lastIndex);

	_SortTree(tree, oneLevelOnly, compareFunc);

	// Populate to the full list
	_PopulateTree(tree, fFullList, firstIndex, false);

	if (superItem == NULL
		|| (superItem->IsItemVisible() && superItem->IsExpanded())) {
		// Populate to BListView's list
		firstIndex = fList.IndexOf(superItem) + 1;
		lastIndex = firstIndex;
		_PopulateTree(tree, fList, lastIndex, true);

		if (fFirstSelected != -1) {
			// update selection hints
			fFirstSelected = _CalcFirstSelected(0);
			fLastSelected = _CalcLastSelected(CountItems());
		}

		// only invalidate what may have changed
		_RecalcItemTops(firstIndex);
		BRect top = ItemFrame(firstIndex);
		BRect bottom = ItemFrame(lastIndex - 1);
		BRect update(top.left, top.top, bottom.right, bottom.bottom);
		Invalidate(update);
	}

	_DestructTree(tree);
}


int32
BOutlineListView::CountItemsUnder(BListItem* superItem, bool oneLevelOnly) const
{
	int32 i = FullListIndexOf(superItem);
	if (i == -1)
		return 0;

	++i;
	int32 count = 0;
	uint32 baseLevel = superItem->OutlineLevel();

	for (; i < FullListCountItems(); i++) {
		BListItem* item = FullListItemAt(i);

		// If we jump out of the subtree, return count
		if (item->fLevel <= baseLevel)
			return count;

		// If the level matches, increase count
		if (!oneLevelOnly || item->fLevel == baseLevel + 1)
			count++;
	}

	return count;
}


BListItem*
BOutlineListView::EachItemUnder(BListItem* superItem, bool oneLevelOnly,
	BListItem* (*eachFunc)(BListItem* item, void* arg), void* arg)
{
	int32 i = FullListIndexOf(superItem);
	if (i == -1)
		return NULL;

	i++; // skip the superitem
	while (i < FullListCountItems()) {
		BListItem* item = FullListItemAt(i);

		// If we jump out of the subtree, return NULL
		if (item->fLevel <= superItem->OutlineLevel())
			return NULL;

		// If the level matches, check the index
		if (!oneLevelOnly || item->fLevel == superItem->OutlineLevel() + 1) {
			item = eachFunc(item, arg);
			if (item != NULL)
				return item;
		}

		i++;
	}

	return NULL;
}


BListItem*
BOutlineListView::ItemUnderAt(BListItem* superItem, bool oneLevelOnly,
	int32 index) const
{
	int32 i = FullListIndexOf(superItem);
	if (i == -1)
		return NULL;

	while (i < FullListCountItems()) {
		BListItem* item = FullListItemAt(i);

		// If we jump out of the subtree, return NULL
		if (item->fLevel < superItem->OutlineLevel())
			return NULL;

		// If the level matches, check the index
		if (!oneLevelOnly || item->fLevel == superItem->OutlineLevel() + 1) {
			if (index == 0)
				return item;

			index--;
		}

		i++;
	}

	return NULL;
}


bool
BOutlineListView::DoMiscellaneous(MiscCode code, MiscData* data)
{
	if (code == B_SWAP_OP)
		return _SwapItems(data->swap.a, data->swap.b);

	return BListView::DoMiscellaneous(code, data);
}


void
BOutlineListView::MessageReceived(BMessage* msg)
{
	BListView::MessageReceived(msg);
}


void BOutlineListView::_ReservedOutlineListView1() {}
void BOutlineListView::_ReservedOutlineListView2() {}
void BOutlineListView::_ReservedOutlineListView3() {}
void BOutlineListView::_ReservedOutlineListView4() {}


void
BOutlineListView::ExpandOrCollapse(BListItem* item, bool expand)
{
	if (item->IsExpanded() == expand || !FullListHasItem(item))
		return;

	item->fExpanded = expand;

	// TODO: merge these cases together, they are pretty similar

	if (expand) {
		uint32 level = item->fLevel;
		int32 fullListIndex = FullListIndexOf(item);
		int32 index = IndexOf(item) + 1;
		int32 startIndex = index;
		int32 count = FullListCountItems() - fullListIndex - 1;
		BListItem** items = (BListItem**)fFullList.Items() + fullListIndex + 1;

		BFont font;
		GetFont(&font);
		while (count-- > 0) {
			item = items[0];
			if (item->fLevel <= level)
				break;

			if (!item->IsItemVisible()) {
				// fix selection hints
				if (index <= fFirstSelected)
					fFirstSelected++;
				if (index <= fLastSelected)
					fLastSelected++;

				fList.AddItem(item, index++);
				item->Update(this, &font);
				item->SetItemVisible(true);
			}

			if (item->HasSubitems() && !item->IsExpanded()) {
				// Skip hidden children
				uint32 subLevel = item->fLevel;
				items++;

				while (count > 0 && items[0]->fLevel > subLevel) {
					items++;
					count--;
				}
			} else
				items++;
		}
		_RecalcItemTops(startIndex);
	} else {
		// collapse
		const uint32 level = item->fLevel;
		const int32 fullListIndex = FullListIndexOf(item);
		const int32 index = IndexOf(item);
		int32 max = FullListCountItems() - fullListIndex - 1;
		int32 count = 0;
		bool selectionChanged = false;

		BListItem** items = (BListItem**)fFullList.Items() + fullListIndex + 1;

		while (max-- > 0) {
			item = items[0];
			if (item->fLevel <= level)
				break;

			if (item->IsItemVisible()) {
				fList.RemoveItem(item);
				item->SetItemVisible(false);
				if (item->IsSelected()) {
					selectionChanged = true;
					item->Deselect();
				}
				count++;
			}

			items++;
		}

		_RecalcItemTops(index);
		// fix selection hints
		// if the selected item was just removed by collapsing, select its
		// parent
		if (selectionChanged) {
			if (fFirstSelected > index && fFirstSelected <= index + count) {
					fFirstSelected = index;
			}
			if (fLastSelected > index && fLastSelected <= index + count) {
				fLastSelected = index;
			}
		}
		if (index + count < fFirstSelected) {
				// all items removed were higher than the selection range,
				// adjust the indexes to correspond to their new visible positions
				fFirstSelected -= count;
				fLastSelected -= count;
		}

		int32 maxIndex = fList.CountItems() - 1;
		if (fFirstSelected > maxIndex)
			fFirstSelected = maxIndex;

		if (fLastSelected > maxIndex)
			fLastSelected = maxIndex;

		if (selectionChanged)
			Select(fFirstSelected, fLastSelected);
	}

	_FixupScrollBar();
	Invalidate();
}


BRect
BOutlineListView::LatchRect(BRect itemRect, int32 level) const
{
	float latchWidth = be_plain_font->Size();
	float latchHeight = be_plain_font->Size();
	float indentOffset = level * be_control_look->DefaultItemSpacing();
	float heightOffset = itemRect.Height() / 2 - latchHeight / 2;

	return BRect(0, 0, latchWidth, latchHeight)
		.OffsetBySelf(itemRect.left, itemRect.top)
		.OffsetBySelf(indentOffset, heightOffset);
}


void
BOutlineListView::DrawLatch(BRect itemRect, int32 level, bool collapsed,
	bool highlighted, bool misTracked)
{
	BRect latchRect(LatchRect(itemRect, level));
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	int32 arrowDirection = collapsed ? BControlLook::B_RIGHT_ARROW
		: BControlLook::B_DOWN_ARROW;

	float tintColor = B_DARKEN_4_TINT;
	if (base.red + base.green + base.blue <= 128 * 3) {
		tintColor = B_LIGHTEN_2_TINT;
	}

	be_control_look->DrawArrowShape(this, latchRect, itemRect, base,
		arrowDirection, 0, tintColor);
}


void
BOutlineListView::DrawItem(BListItem* item, BRect itemRect, bool complete)
{
	if (item->fHasSubitems) {
		DrawLatch(itemRect, item->fLevel, !item->IsExpanded(),
			item->IsSelected() || complete, false);
	}

	itemRect.left += LatchRect(itemRect, item->fLevel).right;
	BListView::DrawItem(item, itemRect, complete);
}


int32
BOutlineListView::_FullListIndex(int32 index) const
{
	BListItem* item = ItemAt(index);

	if (item == NULL)
		return -1;

	return FullListIndexOf(item);
}


void
BOutlineListView::_PopulateTree(BList* tree, BList& target,
	int32& firstIndex, bool onlyVisible)
{
	BListItem** items = (BListItem**)target.Items();
	int32 count = tree->CountItems();

	for (int32 index = 0; index < count; index++) {
		BListItem* item = (BListItem*)tree->ItemAtFast(index);

		items[firstIndex++] = item;

		if (item->HasSubitems() && (!onlyVisible || item->IsExpanded())) {
			_PopulateTree(item->fTemporaryList, target, firstIndex,
				onlyVisible);
		}
	}
}


void
BOutlineListView::_SortTree(BList* tree, bool oneLevelOnly,
	int (*compareFunc)(const BListItem* a, const BListItem* b))
{
	BListItem** items = (BListItem**)tree->Items();
	std::sort(items, items + tree->CountItems(),
		ListItemComparator(compareFunc));

	if (oneLevelOnly)
		return;

	for (int32 index = tree->CountItems(); index-- > 0;) {
		BListItem* item = (BListItem*)tree->ItemAt(index);

		if (item->HasSubitems())
			_SortTree(item->fTemporaryList, false, compareFunc);
	}
}


void
BOutlineListView::_DestructTree(BList* tree)
{
	for (int32 index = tree->CountItems(); index-- > 0;) {
		BListItem* item = (BListItem*)tree->ItemAt(index);

		if (item->HasSubitems())
			_DestructTree(item->fTemporaryList);
	}

	delete tree;
}


BList*
BOutlineListView::_BuildTree(BListItem* superItem, int32& fullListIndex)
{
	int32 fullCount = FullListCountItems();
	uint32 level = superItem != NULL ? superItem->OutlineLevel() + 1 : 0;
	BList* list = new BList;
	if (superItem != NULL)
		superItem->fTemporaryList = list;

	while (fullListIndex < fullCount) {
		BListItem* item = FullListItemAt(fullListIndex);

		// If we jump out of the subtree, break out
		if (item->fLevel < level)
			break;

		// If the level matches, put them into the list
		// (we handle the case of a missing sublevel gracefully)
		list->AddItem(item);
		fullListIndex++;

		if (item->HasSubitems()) {
			// we're going deeper
			_BuildTree(item, fullListIndex);
		}
	}

	return list;
}


void
BOutlineListView::_CullInvisibleItems(BList& list)
{
	int32 index = 0;
	while (index < list.CountItems()) {
		if (reinterpret_cast<BListItem*>(list.ItemAt(index))->IsItemVisible())
			++index;
		else
			list.RemoveItem(index);
	}
}


bool
BOutlineListView::_SwapItems(int32 first, int32 second)
{
	// same item, do nothing
	if (first == second)
		return true;

	// fail, first item out of bounds
	if ((first < 0) || (first >= CountItems()))
		return false;

	// fail, second item out of bounds
	if ((second < 0) || (second >= CountItems()))
		return false;

	int32 firstIndex = min_c(first, second);
	int32 secondIndex = max_c(first, second);
	BListItem* firstItem = ItemAt(firstIndex);
	BListItem* secondItem = ItemAt(secondIndex);
	BList firstSubItems, secondSubItems;

	if (Superitem(firstItem) != Superitem(secondItem))
		return false;

	if (!firstItem->IsItemVisible() || !secondItem->IsItemVisible())
		return false;

	int32 fullFirstIndex = _FullListIndex(firstIndex);
	int32 fullSecondIndex = _FullListIndex(secondIndex);
	_GetSubItems(fFullList, firstSubItems, firstItem, fullFirstIndex + 1);
	_GetSubItems(fFullList, secondSubItems, secondItem, fullSecondIndex + 1);
	_DoSwap(fFullList, fullFirstIndex, fullSecondIndex, &firstSubItems,
		&secondSubItems);

	_CullInvisibleItems(firstSubItems);
	_CullInvisibleItems(secondSubItems);
	_DoSwap(fList, firstIndex, secondIndex, &firstSubItems,
		&secondSubItems);

	_RecalcItemTops(firstIndex);
	_RescanSelection(firstIndex, secondIndex + secondSubItems.CountItems());
	Invalidate(Bounds());

	return true;
}


/*!	\brief Removes a single item from the list and all of its children.

	Unlike the BeOS version, this one will actually delete the children, too,
	as there should be no reference left to them. This may cause problems for
	applications that actually take the misbehaviour of the Be classes into
	account.
*/
BListItem*
BOutlineListView::_RemoveItem(BListItem* item, int32 fullListIndex)
{
	if (item == NULL || fullListIndex < 0
		|| fullListIndex >= FullListCountItems()) {
		return NULL;
	}

	uint32 level = item->OutlineLevel();
	int32 superIndex;
	BListItem* super = _SuperitemForIndex(fullListIndex, level, &superIndex);

	if (item->IsItemVisible()) {
		// remove children, too
		while (fullListIndex + 1 < FullListCountItems()) {
			BListItem* subItem = FullListItemAt(fullListIndex + 1);

			if (subItem->OutlineLevel() <= level)
				break;

			if (subItem->IsItemVisible())
				BListView::RemoveItem(subItem);

			fFullList.RemoveItem(fullListIndex + 1);
			delete subItem;
		}
		BListView::RemoveItem(item);
	}

	fFullList.RemoveItem(fullListIndex);

	if (super != NULL) {
		// we might need to change the fHasSubitems field of the parent
		BListItem* child = FullListItemAt(superIndex + 1);
		if (child == NULL || child->OutlineLevel() <= super->OutlineLevel())
			super->fHasSubitems = false;
	}

	return item;
}


/*!	Returns the super item before the item specified by \a fullListIndex
	and \a level.
*/
BListItem*
BOutlineListView::_SuperitemForIndex(int32 fullListIndex, int32 level,
	int32* _superIndex)
{
	BListItem* item;
	fullListIndex--;

	while (fullListIndex >= 0) {
		if ((item = FullListItemAt(fullListIndex))->OutlineLevel()
				< (uint32)level) {
			if (_superIndex != NULL)
				*_superIndex = fullListIndex;
			return item;
		}

		fullListIndex--;
	}

	return NULL;
}


int32
BOutlineListView::_FindPreviousVisibleIndex(int32 fullListIndex)
{
	fullListIndex--;

	while (fullListIndex >= 0) {
		if (FullListItemAt(fullListIndex)->fVisible)
			return fullListIndex;

		fullListIndex--;
	}

	return -1;
}
