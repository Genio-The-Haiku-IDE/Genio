/*
 * Copyright 2023 Nexus6 (nexus6.haiku@icloud.com)
 * Original code taken from Seeker. Copyright by DarkWyrm
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>

#include <Alert.h>
#include <Node.h>
#include <PathMonitor.h>

#include "Log.h"
#include "NodeMonitor.h"
#include "Utils.h"

NodeMonitor::NodeMonitor()
 : BHandler("NodeMonitor")
{
	fHandlers.clear();
}

NodeMonitor::~NodeMonitor()
{
}

status_t
NodeMonitor::StartWatching(const char* path)
{
	status_t status;
	status = BPrivate::BPathMonitor::StartWatching(path, B_WATCH_RECURSIVELY, BMessenger(this));
	if (status != B_OK)
	{
		LogError("Can't start PathMonitor!");
		return B_ERROR;
	}
	return B_OK;
}

status_t
NodeMonitor::StopWatching(const char* path)
{
	status_t status;
	status = BPrivate::BPathMonitor::StopWatching(path, BMessenger(this));
	if (status != B_OK)
	{
		LogError("Can't stop PathMonitor!");
		return B_ERROR;
	}
	return B_OK;
}

// message handler
void NodeMonitor::MessageReceived(BMessage *message)
{
	if(message==NULL)
		return;

	// this should really be a BMessageFilter
	if(message->what!=B_NODE_MONITOR)
		return;

OKAlert("NodeMonitor::MessageReceived", BString("what:")<<message->what,B_WARNING_ALERT);

	int32 op;
	entry_ref toEntryRef, fromEntryRef; 
	const char *name;
	
	message->FindInt32("opcode", &op);
	message->FindInt32("device", &fromEntryRef.device);

	
	if(op==B_ENTRY_MOVED)
	{
		LogTrace("NodeMonitor::MessageReceived B_ENTRY_MOVED");
		message->FindInt64("from directory", &fromEntryRef.directory); 
		message->FindString("name", &name);
		fromEntryRef.set_name(name);

		message->FindInt32("device", &toEntryRef.device);
		message->FindInt64("to directory", &toEntryRef.directory); 
		toEntryRef.set_name(name);
	}
	else
	{
		message->FindInt64("directory", &fromEntryRef.directory); 
		message->FindString("name", &name);
		fromEntryRef.set_name(name);
	}

	if(op==B_STAT_CHANGED)
		message->FindInt64("node", &fromEntryRef.directory);

	// call delegates
	switch(op)
	{
		case B_STAT_CHANGED:
		{
			LogTrace("NodeMonitor::MessageReceived B_STAT_CHANGED");
			for (auto const *x : fHandlers)
				x->OnStatChanged(&fromEntryRef);
			break;
		}
		case B_ENTRY_CREATED:
		{
			LogTrace("NodeMonitor::MessageReceived B_ENTRY_CREATED");
			for (auto const *x : fHandlers)
				x->OnCreated(&fromEntryRef);
			break;
		}
		case B_ENTRY_REMOVED:
		{
			LogTrace("NodeMonitor::MessageReceived B_ENTRY_REMOVED");
			for (auto const *x : fHandlers)
				x->OnRemoved(&fromEntryRef);
			break;
		}
		case B_ENTRY_MOVED:
		{
			LogTrace("NodeMonitor::MessageReceived B_ENTRY_MOVED");
			for (auto const *x : fHandlers)
				x->OnMoved(&fromEntryRef, &toEntryRef);
			break;
		}
		default:
			break;
	}
}

status_t 
NodeMonitor::AddMonitorHandler(INodeMonitorHandler *handler)
{
	if (handler) {
		fHandlers.emplace_back(handler);
	} else {
		return B_ERROR;
	}
	return B_OK;
}

status_t 
NodeMonitor::RemoveMonitorHandler(INodeMonitorHandler *handler)
{
	if (handler) {
		fHandlers.remove(handler);
	} else {
		return B_ERROR;
	}
	return B_OK;
}