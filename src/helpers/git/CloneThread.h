/*
 * Copyright 2023 Nexus6 
 *
 * Adapted from Expander::ExpanderThread class
 * Copyright 2004-2010, Jérôme Duval. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 * Original code from ZipOMatic by jonas.sundstrom@kirilla.com
 */

#pragma once

#include <Message.h>
#include <Messenger.h>
#include <Notification.h>
#include <String.h>
#include <SupportDefs.h>

#include <stdio.h>
#include <stdlib.h>

#include <git2.h>

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <cstdio>
#include <map>
#include <memory>

#include "GenericThread.h"
#include "GitRepository.h"

using namespace std;
using namespace std::filesystem;

namespace Genio::Git {

	enum {
		THREAD_EXIT			= 'prcl',
	};
	
	class CloneThread: public GenericThread {
	public:
									CloneThread(const string& url, const path& localPath);
									~CloneThread();

	private:			
		status_t					ExecuteUnit() override;
		status_t					ThreadShutdown() override;
		
		static int 					_ProgressCallback(const git_transfer_progress *stats, void *payload);
				
		void						_ShowProgressNotification(const BString& content);
		void						_ShowErrorNotification(const BString& content);
		
		template<class E>
		void						_StoreException(E e);
		
		const BString				fThreadUUID;
		const BString				fProgressNotificationMessageID;
		float						fProgress;
		const string				fURL;
		const path 					fLocalPath;
		std::exception_ptr			fLastException;
	};

}