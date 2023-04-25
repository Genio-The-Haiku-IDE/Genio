/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 *
 * Adapted from Expander::ExpanderThread class
 * Copyright 2004-2010, Jérôme Duval. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 * Original code from ZipOMatic by jonas.sundstrom@kirilla.com
 */

#ifndef DIRECTORY_SCAN_THREAD_H
#define DIRECTORY_SCAN_THREAD_H

#include <Message.h>
#include <Messenger.h>
#include <Notification.h>
<<<<<<< HEAD
<<<<<<< HEAD
=======
#include <OutlineListView.h>
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
=======
>>>>>>> 245779f (Introduced scan filters to exclude certain directories from the tree)
#include <String.h>
#include <SupportDefs.h>

#include <stdio.h>
#include <stdlib.h>

#include "FileTreeItem.h"
#include "GenericThread.h"
<<<<<<< HEAD
<<<<<<< HEAD
#include "ProjectTreeView.h"


class DirectoryScanThread: public GenericThread {
public:
								DirectoryScanThread(const entry_ref& ref, ScanRefFilter* filter,
														ProjectTreeView *listView);
=======
=======
#include "ProjectTreeView.h"
>>>>>>> 245779f (Introduced scan filters to exclude certain directories from the tree)


class DirectoryScanThread: public GenericThread {
public:
<<<<<<< HEAD
								DirectoryScanThread(const entry_ref& ref, 
														BOutlineListView *listView);
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
=======
								DirectoryScanThread(const entry_ref& ref, ScanRefFilter* filter,
														ProjectTreeView *listView);
>>>>>>> 245779f (Introduced scan filters to exclude certain directories from the tree)
								~DirectoryScanThread();

private:
			status_t			ExecuteUnit(void) override;
			status_t			ThreadShutdown() override;
			
<<<<<<< HEAD
<<<<<<< HEAD
			void				_ShowProgressNotification(const BString& content);
			void				_ShowErrorNotification(const BString& content);
			
=======
			void				_SetNotifications();
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
=======
			void				_ShowProgressNotification(const BString& content);
			void				_ShowErrorNotification(const BString& content);
			
>>>>>>> 245779f (Introduced scan filters to exclude certain directories from the tree)
			int					_CountEntries(entry_ref* ref);
			FileTreeItem*		_RecursiveScan(const entry_ref* ref, FileTreeItem* item);

private:
<<<<<<< HEAD
<<<<<<< HEAD
			entry_ref* 			fRootRef;
			const BString		fThreadUUID;
			const BString		fProgressNotificationMessageID;
			ProjectTreeView*	fProjectTreeView;
			ScanRefFilter* 		fScanFilter;
			int					fTotalEntries;
			int					fEntryCount;
			float				fScanProgress;
=======
			const BMessenger*	fTarget;
=======
>>>>>>> 245779f (Introduced scan filters to exclude certain directories from the tree)
			entry_ref* 			fRootRef;
			const BString		fThreadUUID;
			const BString		fProgressNotificationMessageID;
			ProjectTreeView*	fProjectTreeView;
			ScanRefFilter* 		fScanFilter;
			int					fTotalEntries;
			int					fEntryCount;
<<<<<<< HEAD
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
=======
			float				fScanProgress;
>>>>>>> 245779f (Introduced scan filters to exclude certain directories from the tree)
};


#endif	// DIRECTORY_SCAN_THREAD_H
