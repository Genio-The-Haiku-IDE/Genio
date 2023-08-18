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

#include "CloneThread.h"
#include "GenioNamespace.h"
#include "Log.h"
#include "TextUtils.h"
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CloneThread"

namespace Genio::Git {

	static std::map<float,bool> fProgressTracker;

	CloneThread::CloneThread(const string& url, const path& localPath)
		: 
		GenericThread(BString("Genio git clone")),
		fThreadUUID(BPrivate::BUuid().SetToRandom().ToString()),
		fProgressNotificationMessageID(fThreadUUID),
		fProgress(0),
		fURL(url),
		fLocalPath(localPath),
		fLastException(nullptr)
	{
	}

	CloneThread::~CloneThread()
	{
	}

	status_t
	CloneThread::ExecuteUnit(void)
	{
		// auto begin = std::chrono::steady_clock::now();
		
		status_t status = B_OK;
		
		_ShowProgressNotification(B_TRANSLATE("Calculating size..."));
		
		git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
		git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
		callbacks.transfer_progress = &CloneThread::_ProgressCallback;
		clone_opts.fetch_opts.callbacks = callbacks;
        
		git_repository *fRepository = nullptr;
		
		int error = git_clone(&fRepository, fURL.c_str(), fLocalPath.c_str(), &clone_opts);
		if (error != 0) {
			_StoreException(GitException(error, git_error_last()->message));
			// throw GitException(error, git_error_last()->message);
		}

		Quit();
		
		// auto end = std::chrono::steady_clock::now();
		// auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
		// LogTrace("CloneThread: Elapsed period: %ld microseconds\n", elapsed);
		
		return status;
	}

	status_t
	CloneThread::ThreadShutdown()
	{
		_ShowProgressNotification(B_TRANSLATE("Completed"));
		
		// we can't use the callback function if we need to invoke a function that must be called from
		// within the same thread of the Looper to which the handler belongs to (i.e. PathMonitor)
		// Both options are available for maximum flexibility
		// BMessage *message = new BMessage(Genio::Git::THREAD_EXIT);
		// message->AddPointer("exception", (void *)fLastException->);
		// BMessenger messenger(fTarget);
		// messenger.SendMessage(message);
		return B_OK;
	}

	void
	CloneThread::_ShowProgressNotification(const BString& content)
	{
		ProgressNotification("Genio", BString(" "), 
								fProgressNotificationMessageID, content, fProgress);
	}

	void
	CloneThread::_ShowErrorNotification(const BString& content)
	{
		// ErrorNotification("Genio", " ", "", content);
	}
	
	int 
	CloneThread::_ProgressCallback(const git_transfer_progress *stats, void *payload) 
	{	
		int timeout = -1;
		int current_progress = stats->total_objects > 0 ?
			(100*stats->received_objects) /
			stats->total_objects :
			0;
		int kbytes = stats->received_bytes / 1024;
		
		BString progressString;
		progressString << "Network " << current_progress 
			<< " (" << kbytes << " kb, "
			<< stats->received_objects << "/"
			<< stats->total_objects << ")\n";

		std::cout << "Current progress: " << current_progress << "% - " << stats->received_objects << " objects received out of " << stats->total_objects << "\n";
		
		
		if (fProgressTracker[current_progress] == false) {
			if (current_progress == 100)
				timeout = -1;
			ProgressNotification("Genio", BString("Clone remote repository"), 
									"GitRepositoryClone", 
									progressString, 
									(float)current_progress/100, timeout);
			fProgressTracker[current_progress] = true;
		}
			
        return 0;
    }
	
	template<class E>
	void
	CloneThread::_StoreException(E e)
	{
		fLastException = make_exception_ptr<E>(e);
	}
	

}