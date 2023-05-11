/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 */

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Directory.h>
#include <Messenger.h>
#include <NaturalCompare.h>
#include <Uuid.h>

#include <chrono>
#include <errno.h>
#include <filesystem>
#include <functional>
#include <image.h>
#include <iostream>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "DirectoryScanThread.h"
#include "GenioNamespace.h"
#include "Log.h"
#include "TextUtils.h"
#include "ProjectTreeView.h"
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DirectoryScanThread"

DirectoryScanThread::DirectoryScanThread(const entry_ref& ref, ScanRefFilter* filter,
											ProjectTreeView *listView)
	: 
	GenericThread(BString("Genio project scanner")),
	fRootRef(new entry_ref(ref)),
	fThreadUUID(BPrivate::BUuid().SetToRandom().ToString()),
	fProgressNotificationMessageID(fThreadUUID),
	fProjectTreeView(listView),
	fScanFilter(filter),
	fTotalEntries(0),
	fEntryCount(0)
{
}

DirectoryScanThread::~DirectoryScanThread()
{
}

status_t
DirectoryScanThread::ExecuteUnit(void)
{
	// auto begin = std::chrono::steady_clock::now();
	
	status_t status = B_OK;
	
	_ShowProgressNotification(B_TRANSLATE("Calculating size..."));
	
	fTotalEntries = _CountEntries(fRootRef);
	LogTrace("_CountEntries(%s) = %d", fRootRef->name, fTotalEntries);
	fEntryCount = 0;

	_ShowProgressNotification(B_TRANSLATE("Loading project..."));
	
	FileTreeItem *rootItem = _RecursiveScan(fRootRef, nullptr);
	if (rootItem->InitCheck() == B_OK) {
		// TODO: update_mime_info should be run only on demand (context menu or project settings?)
		// update_mime_info(rootItem->GetStringPath()->String(), true, false, B_UPDATE_MIME_INFO_NO_FORCE);
		fProjectTreeView->SortTree(rootItem);
	} else {
		_ShowErrorNotification("An error occurred while loading the project");
		fProjectTreeView->RemoveItem(rootItem);
	}
	Quit();
	
	// auto end = std::chrono::steady_clock::now();
	// auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
	// LogTrace("DirectoryScanThread: Elapsed period: %ld microseconds\n", elapsed);
	
	return status;
}

status_t
DirectoryScanThread::ThreadShutdown()
{
	_ShowProgressNotification(B_TRANSLATE("Completed"));
	
	// we can't use the callback function if we need to invoke a function that must be called from
	// within the same thread of the Looper to which the handler belongs to (i.e. PathMonitor)
	// Both options are available for maximum flexibility
	BMessage *message = new BMessage(ProjectTreeView::ProjectTreeMessage::ON_ROOT_ITEM_ADDED);
	message->AddRef("refs", fRootRef);
	BMessenger messenger(fProjectTreeView);
	messenger.SendMessage(message);
	return B_OK;
}

void
DirectoryScanThread::_ShowProgressNotification(const BString& content)
{
	ProgressNotification("Genio", BString("Project ") << fRootRef->name, 
							fProgressNotificationMessageID, content, fScanProgress);
}

void
DirectoryScanThread::_ShowErrorNotification(const BString& content)
{
	ErrorNotification("Genio", BString("Project ") << fRootRef->name, 
							fRootRef->name, content);
}

int
DirectoryScanThread::_CountEntries(entry_ref* ref)
{
	int count = 0;
	BEntry nextEntry;
	BEntry entry(ref);
	bool doScan = true;
	
	if (fScanFilter != nullptr) {
		doScan = fScanFilter->Filter(ref, nullptr, nullptr, nullptr);
		if (doScan) {
			if (entry.IsDirectory())
			{
				entry_ref nextRef;
				BDirectory dir(&entry);
				count = dir.CountEntries();
				while(dir.GetNextEntry(&nextEntry)==B_OK)
				{
					nextEntry.GetRef(&nextRef);
					count += _CountEntries(&nextRef);
				}
			}
		} else {
			LogTrace("_CountEntries(%s) skip ref=%s doScan=%d count=%d", fRootRef->name, ref->name, doScan, count);
			std::filesystem::path x;
		}
	}
	
	return count;
}

FileTreeItem*
DirectoryScanThread::_RecursiveScan(const entry_ref* ref, FileTreeItem* item)
{
	BEntry nextEntry;
	BEntry entry(ref);
	bool doScan = true;
	
	if (fScanFilter != nullptr)
		doScan = fScanFilter->Filter(ref, nullptr, nullptr, nullptr);
		
	FileTreeItem *newItem = fProjectTreeView->CreateItem(ref);
	
	if (item!=nullptr) {
		if (doScan) {
			newItem->SetParentItem(item);
			fProjectTreeView->AddUnder(newItem,item);
			fProjectTreeView->Collapse(newItem);
			fEntryCount++;
		} else {
			LogTrace("_CountEntries() skip ref=%s doScan=false", ref->name);
		}
	} else {
		// this is the first recursive call and we need to create the superitem
		// it is then returned back to caller for subsequent operations
		newItem->SetAsRoot(true);
		fProjectTreeView->AddItem(newItem);
		fProjectTreeView->Collapse(newItem);
		item = newItem;
	}
	
	if (entry.IsDirectory())
	{
		if (doScan) {
			entry_ref nextRef;
			BDirectory dir(&entry);
			dir.Rewind();
			status_t status;
			while((status = dir.GetNextEntry(&nextEntry))==B_OK)
			{
				nextEntry.GetRef(&nextRef);
				_RecursiveScan(&nextRef, newItem);
			}
		} else {
			LogTrace("_CountEntries() entry.IsDirectory() skip ref=%s doScan=false", ref->name);
		}
	}
	
	// calculate progress and updates BNotification
	// We need to avoid bombarding the Notification server with too many requests so we do it
	// only with increments of 10% by using the custom Round() function with a precision of 
	// 1 decimal digit
	float cProgress = Round((float)fEntryCount/(float)fTotalEntries,1);
	if (cProgress != fScanProgress) {
		_ShowProgressNotification(BString("") << fEntryCount << "/" << fTotalEntries 
									<< " files scanned.");
		fScanProgress = cProgress;
	}
	
	return item;
} 