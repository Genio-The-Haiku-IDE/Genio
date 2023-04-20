/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

<<<<<<< HEAD
=======
// void
// ProjectTreeView::()
// {
// }

>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Mime.h>
#include <NaturalCompare.h>

<<<<<<< HEAD
#include <algorithm>

#include "Log.h"
#include "ProjectTreeView.h"
#include "DirectoryScanThread.h"
=======
#include <thread>

#include "Log.h"
#include "ProjectTreeView.h"
<<<<<<< HEAD
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
=======
#include "DirectoryScanThread.h"
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProjectTreeView"

<<<<<<< HEAD
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



=======
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
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
	
<<<<<<< HEAD
=======
	// AddChild(fFileTreeView);
	// AddChild(fFileSearchControl);
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
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
<<<<<<< HEAD
ProjectTreeView::AddRootItem(const entry_ref& ref, ScanRefFilter* filter)
{
	auto thread = new DirectoryScanThread(ref, filter, this);
	thread->Start();
	
=======
ProjectTreeView::AddRootItem(const entry_ref& ref, BRefFilter* filter)
{
	// status_t status;
	
	// auto lambda = [=]{ _ScanThread(ref); };
	// std::thread scanThread(&ProjectTreeView::_ScanThread, this, ref);
	// std::thread scanThread(lambda);
	// scanThread.detach();

	auto thread = new DirectoryScanThread(ref, fFileTreeView);
	thread->Start();
	
	// _ScanThread(ref);
	
	// if (status != B_OK)
		// LogError("(AddRootItem) unable to start scan thread for [%s]", directory->name);
	// return status;
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
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
<<<<<<< HEAD
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

=======
ProjectTreeView::_MoveUnder(BListItem* item, BListItem* superitem, bool moveChildren)
{
}

>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
void
ProjectTreeView::FilterAllItemsByText(const BString& search)
{
}

BListItem*
ProjectTreeView::_FindItemByRef(const entry_ref& ref)
{
	return nullptr;
}

<<<<<<< HEAD
=======
int32
ProjectTreeView::_ScanThread(const entry_ref& ref)
{
	// TODO: use some sort of GUID to add to messageID to avoid clashes with 
	// projects with the same name in different paths
	BNotification notification(B_PROGRESS_NOTIFICATION);
	// notification.SetGroup("Genio");
	// BString title(B_TRANSLATE("Opening project "));
	// title << ref.name; 
	// notification.SetTitle(title);
	// BString messageID = "open_project_progress_";
	// messageID << ref.name;
	// notification.SetMessageID(messageID);
	// notification.SetProgress(0.0);	
	// notification.SetContent(B_TRANSLATE("Calculating size..."));
	// notification.Send();
	
	const int nEntries = CountEntries(ref);
	int currentEntries = 0;
	
	// notification.SetContent(B_TRANSLATE("Loading project..."));
	// notification.Send();
	
	// FileTreeItem *rootItem = _RecursiveScan(ref, nullptr, totalEntries, currentEntries, notification);
	// _RecursiveScan(ref, nullptr, nEntries, currentEntries, notification);
	// if (rootItem->InitCheck() == B_OK) {
		// update_mime_info(rootItem->GetStringPath()->String(), true, false, B_UPDATE_MIME_INFO_NO_FORCE);
		// BAutolock lock(Looper());
		// fFileTreeView->SortItemsUnder(rootItem, false, ProjectTreeView::_CompareProjectItems);
		// fFileTreeView->Invalidate();
	// } else {
		// BString message = "An error occurred while loading ";
		// message << ref.name;
		// (new BAlert("_ScanThread",
						// B_TRANSLATE(message),
						// B_TRANSLATE("OK"), NULL, NULL,
						// B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT))->Go();
		// fFileTreeView->RemoveItem(rootItem);
	// }
	
	// notification.SetProgress(1.0);
	// notification.Send();
	BString message;
	message << "CountEntries = " << ref.name << " " << nEntries << "\n";
	message << "currentEntries = " << ref.name << " " << currentEntries << "\n";
	
			(new BAlert("_ScanThread",
						message,
						B_TRANSLATE("OK"), NULL, NULL,
						B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT))->Go();
	
	return B_OK;
}

FileTreeItem*
ProjectTreeView::_RecursiveScan(const entry_ref& ref, FileTreeItem* item, 
									const int totalEntries,	int& entryCount,
									BNotification& notification)
{
	BEntry nextEntry;
	BEntry entry(&ref);	
				
	FileTreeItem *newItem = new FileTreeItem(ref, fFileTreeView);
	
	if (item!=nullptr) {
		// BAutolock lock(Looper());
		// fFileTreeView->AddUnder(newItem,item);
		// fFileTreeView->Collapse(newItem);
	} else {
		// this is the first recursive call and we need to create the superitem
		// it is then returned back to caller for subsequent operations
		// BAutolock lock(Looper());
		// fFileTreeView->AddItem(newItem);
		// fFileTreeView->Collapse(newItem);
		item = newItem;
	}
	
	if (entry.IsDirectory())
	{
		entry_ref nextRef;
		BDirectory dir(&entry);
		dir.Rewind();
		entryCount += dir.CountEntries();
		while(dir.GetNextEntry(&nextEntry)==B_OK)
		{
			nextEntry.GetRef(&nextRef);
			_RecursiveScan(nextRef, newItem, totalEntries, entryCount, notification);
		}
	}
	
	// calculate progress and updates BNotification
	// float progress = (float)entryCount/(float)totalEntries;
	// BString content;
	// content << entryCount << "/" << totalEntries;
	// notification.SetContent(content);
	// notification.SetProgress(progress);
	// notification.Send();
	
	return item;
}

int
ProjectTreeView::CountEntries(const entry_ref& ref)
{
	int count = 0;
	BEntry nextEntry;
	BEntry entry(&ref);	
				
	if (entry.IsDirectory())
	{
		entry_ref nextRef;
		BDirectory dir(&entry);
		dir.Rewind();
		count = dir.CountEntries();
		while(dir.GetNextEntry(&nextEntry)==B_OK)
		{
			nextEntry.GetRef(&nextRef);
			count += CountEntries(nextRef);
		}
	}
	return count;
}

>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
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