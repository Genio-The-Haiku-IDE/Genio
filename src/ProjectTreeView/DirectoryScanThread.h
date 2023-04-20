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
=======
#include <OutlineListView.h>
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
#include <String.h>
#include <SupportDefs.h>

#include <stdio.h>
#include <stdlib.h>

#include "FileTreeItem.h"
#include "GenericThread.h"
<<<<<<< HEAD
#include "ProjectTreeView.h"


class DirectoryScanThread: public GenericThread {
public:
								DirectoryScanThread(const entry_ref& ref, ScanRefFilter* filter,
														ProjectTreeView *listView);
=======

enum {
	DIRECTORYSCANTHREAD_ENABLE_STOP_BUTTON	= 'Cesb',
	DIRECTORYSCANTHREAD_ERROR				= 'Cerr',
	DIRECTORYSCANTHREAD_EXIT				= 'Cexi',
	DIRECTORYSCANTHREAD_CMD_TYPE			= 'Ccty',
	DIRECTORYSCANTHREAD_PRINT_BANNER		= 'Cpba',
	DIRECTORYSCANTHREAD_STOP				= 'Csto',
	DIRECTORYSCANTHREAD_STDOUT				= 'Csou',
	DIRECTORYSCANTHREAD_STDERR				= 'Cser'
};

class DirectoryScanThread: public GenericThread {
public:
								DirectoryScanThread(const entry_ref& ref, 
														BOutlineListView *listView);
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
								~DirectoryScanThread();

private:
			status_t			ExecuteUnit(void) override;
			status_t			ThreadShutdown() override;
			
<<<<<<< HEAD
			void				_ShowProgressNotification(const BString& content);
			void				_ShowErrorNotification(const BString& content);
			
=======
			void				_SetNotifications();
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
			int					_CountEntries(entry_ref* ref);
			FileTreeItem*		_RecursiveScan(const entry_ref* ref, FileTreeItem* item);

private:
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
			entry_ref* 			fRootRef;
			BNotification*		fProgressNotification;
			BNotification*		fErrorNotification;
			BOutlineListView*	fFileTreeView;
			int					fTotalEntries;
			int					fEntryCount;
>>>>>>> 8b271b4 (Directory traversing is performed as an instance of GenericThread)
};


#endif	// DIRECTORY_SCAN_THREAD_H
