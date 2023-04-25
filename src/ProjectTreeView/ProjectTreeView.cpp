/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Mime.h>
#include <NaturalCompare.h>

#include <thread>

#include "Log.h"
#include "ProjectTreeView.h"
#include "DirectoryScanThread.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectTreeView"

ScanRefFilter::ScanRefFilter()
	:
	BRefFilter()
{
}

bool
ScanRefFilter::AddPath(const entry_ref* ref)
{
	entry_ref *cpRef = new entry_ref(*ref); 
	BPath path(cpRef);
	LogTrace("AddPath(%d) adding %s", fExcluded.CountItems(), path.Path());
	bool status = fExcluded.AddItem(cpRef);
	LogTrace("AddPath(%d) items in the list", fExcluded.CountItems());
	for(int i=0;i<fExcluded.CountItems();i++)
		LogTrace("AddPath(%d) items %s", fExcluded.CountItems(), fExcluded.ItemAt(i)->name);
	return status;
}

bool
ScanRefFilter::RemovePath(const entry_ref* ref)
{
	entry_ref *cpRef = new entry_ref(*ref); 
	BPath path(cpRef);
	LogTrace("AddPath(%d) removing %s", fExcluded.CountItems(), path.Path());
	bool status = false;
	LogTrace("AddPath(%d) items in the list", fExcluded.CountItems());
	for(int i=0;i<fExcluded.CountItems();i++) {
		entry_ref *item = (entry_ref*)fExcluded.ItemAt(i);
		if (item->name == cpRef->name && 
			item->device == cpRef->device && 
			item->directory == cpRef->directory) {
			status = fExcluded.RemoveItem(item);
			LogTrace("AddPath(%d) items %s", fExcluded.CountItems(), fExcluded.ItemAt(i)->name);
		}
	}
	return status;
}

bool
ScanRefFilter::Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat,
						const char* filetype)
{
	BPath path(ref);
	node = new BNode(ref);
	if (node != nullptr && node->IsDirectory()) {
		LogTrace("Filter(): ref(%s) path(%s)", ref->name, path.Path());
		for(int i=0;i<fExcluded.CountItems();i++) {
			entry_ref *item = (entry_ref*)fExcluded.ItemAt(i);
			LogTrace("Filter(): item(%d,%d,%s)",item->device, item->directory,item->name);
			LogTrace("Filter(): ref(%d,%d,%s)",ref->device, ref->directory,ref->name);
			if (BString(item->name)==BString(ref->name) && 
				item->device == ref->device && 
				item->directory == ref->directory) {
				LogTrace("Filter(): HasItem()");
				return false;
			}
		}
	}
	return true;
}



ProjectTreeView::ProjectTreeView(const char* name, uint32 flags, BLayout* layout):
	BView(name, flags, layout),
	fFileTreeView(nullptr),
	fFileSearchControl(nullptr)
{
	fFileTreeView = new BOutlineListView("FileTreeView", 
											B_SINGLE_SELECTION_LIST, flags);
	fFileSearchControl = new BTextControl("FileSearchView", "", "", 
											new BMessage(FILE_SEARCH_MSG), flags);
											
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fFileTreeView)
		.Add(fFileSearchControl);
	
}

ProjectTreeView::~ProjectTreeView()
{
}

void
ProjectTreeView::AttachedToWindow(void)
{
	fFileTreeView->SetTarget(this);
	fFileSearchControl->SetTarget(this);
}

void
ProjectTreeView::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case FILE_SEARCH_MSG: {
			BString search;
			if (msg->FindString("search",&search) == B_OK) {
				FilterAllItemsByText(search);
			}
			break;
		}
		// case XXX: {
		// 
		// 		break;
		// }
		default: {
			BView::MessageReceived(msg);
			break;
		}
	}
}

status_t
ProjectTreeView::AddRootItem(const entry_ref& ref, ScanRefFilter* filter)
{
	auto thread = new DirectoryScanThread(ref, filter, this);
	thread->Start();
	
	return B_OK;
}

void
ProjectTreeView::_OnRootItemAdded(const entry_ref& ref, BListItem* item)
{
}

status_t
ProjectTreeView::RemoveRootItem(const entry_ref& ref)
{
	LogError("(REMOVE_PROJECT) entry_ref not passed with BMessage");
	return B_OK;
}

status_t
ProjectTreeView::ActivateRootItem(const entry_ref& ref)
{
	LogError("(ACTIVATE_PROJECT) entry_ref not passed with BMessage");
	return B_OK;
}

status_t
ProjectTreeView::DeactivateRootItem(const entry_ref& ref)
{
	LogError("(DEACTIVATE_PROJECT) entry_ref not passed with BMessage");
	return B_OK;
}

void
ProjectTreeView::MoveUnder(BListItem* item, BListItem* superitem, bool moveChildren)
{
}

bool
ProjectTreeView::AddItem(BListItem *item)
{
	BAutolock lock(fFileTreeView->Looper());
	return fFileTreeView->AddItem(item);
}

bool
ProjectTreeView::AddUnder(BListItem *item, BListItem *superItem)
{
	BAutolock lock(fFileTreeView->Looper());
	return fFileTreeView->AddUnder(item, superItem);
}

bool
ProjectTreeView::RemoveItem(BListItem *item)
{
	BAutolock lock(fFileTreeView->Looper());
	return fFileTreeView->RemoveItem(item);
}

void
ProjectTreeView::SortTree(BListItem *rootItem)
{
	BAutolock lock(fFileTreeView->Looper());
	fFileTreeView->SortItemsUnder(rootItem, false, ProjectTreeView::_CompareProjectItems);
}

void
ProjectTreeView::Collapse(BListItem *item)
{
	BAutolock lock(fFileTreeView->Looper());
	fFileTreeView->Collapse(item);
}

FileTreeItem*
ProjectTreeView::CreateItem(const entry_ref* ref)
{
	return new FileTreeItem(*ref, fFileTreeView);
}

void
ProjectTreeView::FilterAllItemsByText(const BString& search)
{
}

BListItem*
ProjectTreeView::_FindItemByRef(const entry_ref& ref)
{
	return nullptr;
}

int
ProjectTreeView::_CompareProjectItems(const BListItem* a, const BListItem* b)
{
	if (a == b)
		return 0;

	const FileTreeItem* A = dynamic_cast<const FileTreeItem*>(a);
	const FileTreeItem* B = dynamic_cast<const FileTreeItem*>(b);
	const char* nameA = A->Text();
	const char* nameB = B->Text();
	
	if (A->IsDirectory() && !(B->IsDirectory())) {
		return -1;
	}
	
	if (!(A->IsDirectory()) && B->IsDirectory()) {
		return 1;
	}
		
	if (nameA == NULL) {
		return 1;
	}
		
	if (nameB == NULL) {
		return -1;
	}

	// Natural order sort
	if (nameA != NULL && nameB != NULL)
		return BPrivate::NaturalCompare(nameA, nameB);
		
	return 0;
}