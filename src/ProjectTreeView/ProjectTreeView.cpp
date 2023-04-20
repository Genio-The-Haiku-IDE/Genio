/*
 * Copyright 2023, Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

// void
// ProjectTreeView::()
// {
// }

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
	
	// AddChild(fFileTreeView);
	// AddChild(fFileSearchControl);
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
ProjectTreeView::_MoveUnder(BListItem* item, BListItem* superitem, bool moveChildren)
{
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