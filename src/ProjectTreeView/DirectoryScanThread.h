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
#include <String.h>
#include <SupportDefs.h>

#include <stdio.h>
#include <stdlib.h>

#include "FileTreeItem.h"
#include "GenericThread.h"
#include "ProjectTreeView.h"


class DirectoryScanThread: public GenericThread {
public:
								DirectoryScanThread(const entry_ref& ref, ScanRefFilter* filter,
														ProjectTreeView *listView);
								~DirectoryScanThread();

private:
			status_t			ExecuteUnit(void) override;
			status_t			ThreadShutdown() override;
			
			void				_ShowProgressNotification(const BString& content);
			void				_ShowErrorNotification(const BString& content);
			
			int					_CountEntries(entry_ref* ref);
			FileTreeItem*		_RecursiveScan(const entry_ref* ref, FileTreeItem* item);

private:
			entry_ref* 			fRootRef;
			const BString		fThreadUUID;
			const BString		fProgressNotificationMessageID;
			ProjectTreeView*	fProjectTreeView;
			ScanRefFilter* 		fScanFilter;
			int					fTotalEntries;
			int					fEntryCount;
			float				fScanProgress;
};


#endif	// DIRECTORY_SCAN_THREAD_H
