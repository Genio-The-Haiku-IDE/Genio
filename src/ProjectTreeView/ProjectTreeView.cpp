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

#include <algorithm>

#include "Log.h"
#include "ProjectTreeView.h"
#include "DirectoryScanThread.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectTreeView"

ScanRefFilter::ScanRefFilter(const char* base_path)
	:
	BRefFilter(),
	fBasePath(base_path)
{
}

ScanRefFilter::~ScanRefFilter()
{
	// for (auto const& item: fExcludedList)
		// delete item;
}

void
ScanRefFilter::AddPath(std::filesystem::path relative_path)
{
	// entry_ref *cpRef = new entry_ref(*ref); 
	// LogTrace("AddPath(%d) adding %s", fExcludedList.size(), cpRef->name);
	fExcludedList.push_back(relative_path);
	// LogTrace("AddPath(%d) item %s added", fExcludedList.size(), cpRef->name);
	// LogTrace("AddPath() list of items");
	// for (auto const& item: fExcludedList)
		// LogTrace("AddPath(%s): ", item->name);
}

void
ScanRefFilter::RemovePath(std::filesystem::path relative_path)
{
	// entry_ref *cpRef = new entry_ref(*ref); 
	LogTrace("RemovePath(%d) removing %s", fExcludedList.size(), relative_path.c_str());
	
	auto predicate = [relative_path](std::filesystem::path item) { 
		return 	relative_path == item;
	};
	auto element = std::find_if(fExcludedList.begin(), fExcludedList.end(), predicate);
	if (element != fExcludedList.end()) {
		fExcludedList.erase(element);
		LogTrace("RemovePath(%d) item %s removed", fExcludedList.size(),(*element).c_str());
	} else {
		LogTrace("RemovePath(%d) item %s not found", fExcludedList.size(),(*element).c_str());
	}
	LogTrace("RemovePath() list of items");
	for (auto const& item: fExcludedList)
		LogTrace("RemovePath(%s): ", item.c_str());
}

bool
ScanRefFilter::Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat,
						const char* filetype)
{
	chdir(fBasePath.c_str());
	bool exclude = true;
	entry_ref *cpRef = new entry_ref(*ref);
	node = new BNode(ref);
	if (node != nullptr && node->IsDirectory()) {
		LogTrace("Filter(): ref(%s)", cpRef->name);
		
		auto predicate = [cpRef](std::filesystem::path item) {
			auto absolute_path = std::filesystem::absolute(item);
			BEntry entry(absolute_path.c_str());
			entry_ref itemRef;
			entry.GetRef(&itemRef);
			LogTrace("Filter(predicate): cpRef(%s,%d,%d)", cpRef->name, cpRef->device, cpRef->directory);
			LogTrace("Filter(predicate): item(%s,%d,%d)", itemRef.name, itemRef.device, itemRef.directory);
			return 	BString(cpRef->name) == BString(itemRef.name) &&
					cpRef->device == itemRef.device &&
					cpRef->directory == itemRef.directory;
		};
		auto element = std::find_if(fExcludedList.begin(), fExcludedList.end(), predicate);
		if (element != fExcludedList.end()) {
			LogTrace("Filter() item %s is in the excluded list", (*element).c_str());
			exclude = false;
		} else {
			LogTrace("Filter() item %s is not in the excluded list", cpRef->name);
			exclude = true;
		}
	}
	delete node;
	return exclude;
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