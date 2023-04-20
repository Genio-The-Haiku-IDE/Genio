/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 *
 */

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Messenger.h>
#include <Uuid.h>

#include <errno.h>
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

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DirectoryScanThread"

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
}

DirectoryScanThread::~DirectoryScanThread()
{
	delete fProgressNotification;
}

status_t
DirectoryScanThread::ExecuteUnit(void)
{
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
	return status;
}

status_t
DirectoryScanThread::ThreadShutdown(void)
{
	LogTrace("DirectoryScanThread::Quit(): root_ref(%s) progress(%s)\r", 
				fRootRef->name, (BString("") << fEntryCount << "/" << fTotalEntries).String());
	fProgressNotification->SetProgress(1.0);
	fProgressNotification->Send();
	return B_OK;
}

void
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
}

int
DirectoryScanThread::_CountEntries(entry_ref* ref)
{
	int count = 0;
	BEntry nextEntry;
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
		}
	}
	return count;
}

FileTreeItem*
DirectoryScanThread::_RecursiveScan(const entry_ref* ref, FileTreeItem* item)
{
	static int level = 0;
	
	BEntry nextEntry;
	BEntry entry(ref);	

	FileTreeItem *newItem = new FileTreeItem(*ref, fFileTreeView);
	
	if (item!=nullptr) {
		// BAutolock lock(Looper());
		// fFileTreeView->AddUnder(newItem,item);
		// fFileTreeView->Collapse(newItem);
		fEntryCount++;
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
			LogTrace("DirectoryScanThread::_RecursiveScan: %s(%s)", slevel.String(), snes.String());
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
