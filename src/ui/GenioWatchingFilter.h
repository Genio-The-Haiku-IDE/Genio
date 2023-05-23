/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GenioWatchingFilter_H
#define GenioWatchingFilter_H


#include <PathMonitor.h>
#include "Log.h"

// This is attached to the PathMonitor class to avoid watching too many (useless) files.
// In the ProjectFolderBrowser we care only on directories and related files events.
// We can't use the B_WATCH_DIRECTORIES_ONLY flag as PathMonitor won't notify for file related events.

class GenioWatchingFilter : public BPrivate::BPathMonitor::BWatchingInterface {
public:
	status_t WatchNode(const node_ref* node, uint32 flags, const BHandler* handler,
							  		const BLooper* looper = NULL) 
									
	{
		status_t status;
		BDirectory dir(node);
		if ((status = dir.InitCheck()) != B_OK) {
			// Typically the reason for this failure is "Not a directory".
			// As we want to avoid to use a watch_node for every standard file,
			// we quit here.
			return B_OK;
		}
		
		status = BPrivate::BPathMonitor::BWatchingInterface::WatchNode(node, flags, handler, looper);
		if (status != B_OK) {
			LogErrorF("Can't watch_node for node_id [%ld] (%s)", node->node, strerror(status));
			//TODO: maybe we should notify the user that the PathMonitor is not working?
		}
		return status;
	}
};

#endif // GenioWatchingFilter_H
