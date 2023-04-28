/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
<<<<<<< HEAD
 * All rights reserved. Distributed under the terms of the MIT license.
=======
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
 *
 */

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
<<<<<<< HEAD
#include <Directory.h>
#include <Messenger.h>
#include <NaturalCompare.h>
#include <Uuid.h>

#include <chrono>
#include <errno.h>
#include <filesystem>
=======
#include <Messenger.h>
#include <Uuid.h>

#include <errno.h>
<<<<<<< HEAD
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
=======
#include <filesystem>
>>>>>>> fb8b2aa (ScanRefFilter now supports relative path to folders or files to exclude from scanning. For example, ./.cache or ./.git are still valid when moving the project folder to a different location)
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
<<<<<<< HEAD
#include "Utils.h"
=======
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DirectoryScanThread"

<<<<<<< HEAD
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
=======
DirectoryScanThread::DirectoryScanThread(const entry_ref& ref, 
											BOutlineListView *listView)
	: 
	GenericThread(BString("Genio project scanner_") << BPrivate::BUuid().SetToRandom().ToString())
{
	fProgressNotification = nullptr;
	fErrorNotification = nullptr;
	fFileTreeView = listView;
	fRootRef = new entry_ref(ref);
	_SetNotifications();
	fEntryCount = 0;
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
}

DirectoryScanThread::~DirectoryScanThread()
{
<<<<<<< HEAD
=======
	delete fProgressNotification;
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
}

status_t
DirectoryScanThread::ExecuteUnit(void)
{
<<<<<<< HEAD
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
	
=======
	status_t status = B_OK;
	
	fProgressNotification->SetContent(B_TRANSLATE("Calculating size..."));
	fProgressNotification->Send();
	
	fTotalEntries = _CountEntries(fRootRef);
	fEntryCount = 0;
	
	fProgressNotification->SetContent(B_TRANSLATE("Loading project..."));
	fProgressNotification->Send();
	
	// FileTreeItem *rootItem = _RecursiveScan(fRootRef, nullptr);
	_RecursiveScan(fRootRef, nullptr);
	// FileTreeItem *rootItem = _RecursiveScan(*fRootRef, nullptr, fTotalEntries, fEntryCount, fProgressNotification);
	// if (rootItem->InitCheck() == B_OK) {
		// update_mime_info(rootItem->GetStringPath()->String(), true, false, B_UPDATE_MIME_INFO_NO_FORCE);
		// BAutolock lock(fFileTreeView->Looper());
		// fFileTreeView->SortItemsUnder(rootItem, false, ProjectTreeView::_CompareProjectItems);
		// fFileTreeView->Invalidate();
	// } else {
		// fErrorNotification->Send();
		// fFileTreeView->RemoveItem(rootItem);
	// }
	Quit();
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
	return status;
}

status_t
DirectoryScanThread::ThreadShutdown(void)
{
<<<<<<< HEAD
	// LogTrace("DirectoryScanThread::Quit(): root_ref(%s) progress(%s)\r", 
				// fRootRef->name, (BString("") << fEntryCount << "/" << fTotalEntries).String());
	_ShowProgressNotification(B_TRANSLATE("Completed"));
=======
	LogTrace("DirectoryScanThread::Quit(): root_ref(%s) progress(%s)\r", 
				fRootRef->name, (BString("") << fEntryCount << "/" << fTotalEntries).String());
	fProgressNotification->SetProgress(1.0);
	fProgressNotification->Send();
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
	return B_OK;
}

void
<<<<<<< HEAD
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
=======
DirectoryScanThread::_SetNotifications()
{
	BString title(B_TRANSLATE("Opening project "));
	title << fRootRef->name; 
	BString messageID = "open_project_progress_";
	messageID << BPrivate::BUuid().SetToRandom().ToString();
	
	fProgressNotification = new BNotification(B_PROGRESS_NOTIFICATION);
	fProgressNotification->SetGroup("Genio");
	fProgressNotification->SetTitle(title);
	fProgressNotification->SetMessageID(messageID);
	fProgressNotification->SetProgress(0.0);
	fProgressNotification->Send();
	
	// fErrorNotification = new BNotification(B_ERROR_NOTIFICATION);
	// fErrorNotification->SetGroup("Genio");
	// fErrorNotification->SetTitle(title);
	// fErrorNotification->SetMessageID(messageID);
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
}

int
DirectoryScanThread::_CountEntries(entry_ref* ref)
{
	int count = 0;
	BEntry nextEntry;
<<<<<<< HEAD
<<<<<<< HEAD
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
	
=======
=======
>>>>>>> fb8b2aa (ScanRefFilter now supports relative path to folders or files to exclude from scanning. For example, ./.cache or ./.git are still valid when moving the project folder to a different location)
	BEntry entry(ref);	
				
	if (entry.IsDirectory())
	{
		entry_ref nextRef;
		BDirectory dir(&entry);
		dir.Rewind();
		count = dir.CountEntries();
		while(dir.GetNextEntry(&nextEntry)==B_OK)
		{
			nextEntry.GetRef(&nextRef);
			count += _CountEntries(&nextRef);
=======
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
>>>>>>> f168a64 (ScanRefFilter now supports relative path to folders or files to exclude from scanning. For example, ./.cache or ./.git are still valid when moving the project folder to a different location)
		}
	}
<<<<<<< HEAD
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
=======
	
>>>>>>> fb8b2aa (ScanRefFilter now supports relative path to folders or files to exclude from scanning. For example, ./.cache or ./.git are still valid when moving the project folder to a different location)
	return count;
}

FileTreeItem*
DirectoryScanThread::_RecursiveScan(const entry_ref* ref, FileTreeItem* item)
{
<<<<<<< HEAD
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
=======
	static int level = 0;
	
	BEntry nextEntry;
	BEntry entry(ref);	

	FileTreeItem *newItem = new FileTreeItem(*ref, fFileTreeView);
	
	if (item!=nullptr) {
<<<<<<< HEAD
		// BAutolock lock(Looper());
		// fFileTreeView->AddUnder(newItem,item);
		// fFileTreeView->Collapse(newItem);
		fEntryCount++;
=======
		if (doScan) {
			newItem->SetParentItem(item);
			fProjectTreeView->AddUnder(newItem,item);
			fProjectTreeView->Collapse(newItem);
			fEntryCount++;
		} else {
			LogTrace("_CountEntries() skip ref=%s doScan=false", ref->name);
		}
>>>>>>> f168a64 (ScanRefFilter now supports relative path to folders or files to exclude from scanning. For example, ./.cache or ./.git are still valid when moving the project folder to a different location)
	} else {
		// this is the first recursive call and we need to create the superitem
		// it is then returned back to caller for subsequent operations
		// BAutolock lock(Looper());
		// fFileTreeView->AddItem(newItem);
		// fFileTreeView->Collapse(newItem);
		item = newItem;
		level++;
	}
	
	BString slevel;
	for(int i=0; i<level; i++) slevel+="  ";
	if (entry.IsDirectory())
	{
		entry_ref nextRef;
		BDirectory dir(&entry);
		dir.Rewind();
		LogTrace("DirectoryScanThread::_RecursiveScan: %s+%s (%d/%d)", slevel.String(), ref->name, dir.CountEntries(), fTotalEntries);
		status_t nes;
		while((nes = dir.GetNextEntry(&nextEntry))==B_OK)
		{
			level++;
			nextEntry.GetRef(&nextRef);
			// LogTrace("DirectoryScanThread::_RecursiveScan: \t%s (%d/%d)",nextRef.name, fEntryCount, fTotalEntries);
			// LogTrace("DirectoryScanThread::_RecursiveScan: GetNextEntry(): %s",nextRef.name);
			_RecursiveScan(&nextRef, newItem);
			level--;
			
			BString snes;
			switch (nes) {
				case B_OK: snes = "B_OK"; break;
				case B_BAD_VALUE: snes = "B_BAD_VALUE"; break;
				case B_ENTRY_NOT_FOUND: snes = "B_ENTRY_NOT_FOUND"; break;
				case B_PERMISSION_DENIED: snes = "B_PERMISSION_DENIED"; break;
				case B_NO_MEMORY: snes = "B_NO_MEMORY"; break;
				case B_LINK_LIMIT: snes = "B_LINK_LIMIT"; break;
				case B_BUSY: snes = "B_BUSY"; break;
				case B_FILE_ERROR: snes = "B_FILE_ERROR"; break;
				case B_NO_MORE_FDS: snes = "B_NO_MORE_FDS"; break;
				default: break;
			}
<<<<<<< HEAD
			LogTrace("DirectoryScanThread::_RecursiveScan: %s(%s)", slevel.String(), snes.String());
=======
		} else {
			LogTrace("_CountEntries() entry.IsDirectory() skip ref=%s doScan=false", ref->name);
>>>>>>> f168a64 (ScanRefFilter now supports relative path to folders or files to exclude from scanning. For example, ./.cache or ./.git are still valid when moving the project folder to a different location)
		}
	}
	// debug 
	else {
		LogTrace("DirectoryScanThread::_RecursiveScan: %s%s (%d)", slevel.String(), ref->name, fEntryCount);
	}
	
	// calculate progress and updates BNotification
	// float progress = (float)entryCount/(float)totalEntries;
	// fProgressNotification->SetContent((BString("") << entryCount << totalEntries).String());
	// fProgressNotification->SetProgress(progress);
	// fProgressNotification->Send();
	
	return item;
}
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
