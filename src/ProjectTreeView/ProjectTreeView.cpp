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
#include <PathMonitor.h>

#include <algorithm>

#include "Log.h"
#include "ProjectTreeView.h"
#include "DirectoryScanThread.h"
#include "Utils.h"

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
}

void
ScanRefFilter::AddPath(std::filesystem::path relative_path)
{
	fExcludedList.push_back(relative_path);
}

void
ScanRefFilter::RemovePath(std::filesystem::path relative_path)
{
	// LogTrace("RemovePath(%d) removing %s", fExcludedList.size(), relative_path.c_str());
	
	auto predicate = [relative_path](std::filesystem::path item) { 
		return 	relative_path == item;
	};
	auto element = std::find_if(fExcludedList.begin(), fExcludedList.end(), predicate);
	if (element != fExcludedList.end()) {
		fExcludedList.erase(element);
		// LogTrace("RemovePath(%d) item %s removed", fExcludedList.size(),(*element).c_str());
	} else {
		// LogTrace("RemovePath(%d) item %s not found", fExcludedList.size(),(*element).c_str());
	}
	// LogTrace("RemovePath() list of items");
	// for (auto const& item: fExcludedList)
		// LogTrace("RemovePath(%s): ", item.c_str());
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
		// LogTrace("Filter(): ref(%s)", cpRef->name);
		
		auto predicate = [cpRef](std::filesystem::path item) {
			auto absolute_path = std::filesystem::absolute(item);
			BEntry entry(absolute_path.c_str());
			entry_ref itemRef;
			entry.GetRef(&itemRef);
			// LogTrace("Filter(predicate): cpRef(%s,%d,%d)", cpRef->name, cpRef->device, cpRef->directory);
			// LogTrace("Filter(predicate): item(%s,%d,%d)", itemRef.name, itemRef.device, itemRef.directory);
			return 	BString(cpRef->name) == BString(itemRef.name) &&
					cpRef->device == itemRef.device &&
					cpRef->directory == itemRef.directory;
		};
		auto element = std::find_if(fExcludedList.begin(), fExcludedList.end(), predicate);
		if (element != fExcludedList.end()) {
			// LogTrace("Filter() item %s is in the excluded list", (*element).c_str());
			exclude = false;
		} else {
			// LogTrace("Filter() item %s is not in the excluded list", cpRef->name);
			exclude = true;
		}
	}
	delete node;
	return exclude;
}



ProjectTreeView::ProjectTreeView(const char* name, uint32 flags, BLayout* layout):
	BView(name, flags, layout),
	fFileTreeView(nullptr),
	fFileSearchControl(nullptr),
	fNodeMonitor(nullptr)
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
ProjectTreeView::AttachedToWindow()
{
	fFileTreeView->SetTarget(this);
	fFileSearchControl->SetTarget(this);
	SetNodeMonitor();
}

void
ProjectTreeView::DetachedFromWindow()
{
	BAutolock lock(Looper());
	Looper()->RemoveHandler(fNodeMonitor);
	if (fNodeMonitor != nullptr)
		delete fNodeMonitor;
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
		// case B_PATH_MONITOR: {
			// if (Logger::IsDebugEnabled())
				// msg->PrintToStream(); 			  
			// OKAlert("NodeMonitor", BString("ProjectTreeView::MessageReceived - message->what: ") << msg->what, B_WARNING_ALERT);
			// break;
		// }
		case ON_ROOT_ITEM_ADDED: {
			entry_ref ref;
			if (msg->FindRef("refs", &ref) != B_OK) {
				LogError("ON_ROOT_ITEM_ADDED: refs not found");
				return;
			}
			OnRootItemAdded(&ref);
			break;
		}
		default: {
			BView::MessageReceived(msg);
			break;
		}
	}
}

void 
ProjectTreeView::SetNodeMonitor()
{
	if (LockLooper()) {
		fNodeMonitor = new NodeMonitor();
		Looper()->AddHandler(fNodeMonitor);
		fNodeMonitor->AddMonitorHandler(this);
		auto index = Looper()->IndexOf(fNodeMonitor);
		if (index == B_ERROR) {
			LogTrace("AddHandler: Error adding node monitor handler");
			OKAlert("NodeMonitor", "Error adding node monitor handler", B_WARNING_ALERT);
		}
		UnlockLooper();
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
ProjectTreeView::OnRootItemAdded(const entry_ref* ref)
{
	BPath path(ref);
	if (fNodeMonitor != nullptr) {
		if (fNodeMonitor->StartWatching(path.Path()) != B_OK) {
			LogError("NodeMonitor: Can't watch %s!", path.Path());
			OKAlert("NodeMonitor", BString("Can't watch ") << path.Path(), B_WARNING_ALERT);
		} else {
			LogError("NodeMonitor:OnRootItemAdded watching %s!", path.Path());
			OKAlert("NodeMonitor:OnRootItemAdded", BString("Watching ") << path.Path(), B_WARNING_ALERT);
		}
	}
}

status_t
ProjectTreeView::RemoveRootItem(const entry_ref* ref)
{
	BPath path(ref);
	if (fNodeMonitor->StopWatching(path.Path()) != B_OK) {
		LogError("NodeMonitor: Can't stop watching %s!", path.Path());
		return B_ERROR;
	} else {
		LogError("NodeMonitor: stopped watching %s!", path.Path());
	}
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

void
ProjectTreeView::OnCreated(entry_ref *ref) const {
	OKAlert("ProjectTreeView", BString("ProjectTreeView::OnCreated ") << ref->name, B_WARNING_ALERT);
}

void
ProjectTreeView::OnRemoved(entry_ref *ref) const {
	OKAlert("ProjectTreeView", BString("ProjectTreeView::OnRemoved ") << ref->name, B_WARNING_ALERT);
}

void
ProjectTreeView::OnMoved(entry_ref *origin, entry_ref *destination) const {
	OKAlert("ProjectTreeView", BString("ProjectTreeView::OnMoved from ") << origin->name << " to " << destination->name, B_WARNING_ALERT);
}

void
ProjectTreeView::OnStatChanged(entry_ref *ref) const {
	OKAlert("ProjectTreeView", BString("ProjectTreeView::OnStatChanged ") << ref->name, B_WARNING_ALERT);
}