/*
 * Original code taken from Seeker. Copyright by DarkWyrm
 * Copyright 2023 Nexus6 (nexus6.haiku@icloud.com)
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>

#include <Alert.h>
#include <Node.h>
#include <NodeMonitor.h>

#include "NodeMonitor.h"

NodeMonitor::NodeMonitor()
 : BHandler("NodeMonitor"),
 iWatchCount(0)
{
	fHandlers.clear();
}

NodeMonitor::~NodeMonitor()
{
}

// add a node to the watch list
status_t
NodeMonitor::AddWatch(BMessage *message)
{
	status_t status;
	
	if(message==NULL)
		return B_ERROR;

	node_ref nodeRef;
	status = message->FindInt32("device", &nodeRef.device);
	if(status!=B_OK)
		return status;

	status = message->FindInt64("node", &nodeRef.node);
	if(status!=B_OK)
		return status;

	// add watch
	status = watch_node(&nodeRef, B_WATCH_MOUNT | B_WATCH_NAME | B_WATCH_ATTR | B_STAT_CHANGED | B_WATCH_DIRECTORY, this);
	if(status==B_OK)
		iWatchCount +=1;
	else
		return status;
	
	return B_OK;
}
		
// remove a node from the watch list
status_t
NodeMonitor::RemoveWatch(BMessage *message)
{
	status_t status;
	
	if(message==NULL) {
		return B_ERROR;
	}

	node_ref nodeRef;
	status = message->FindInt32("device", &nodeRef.device);
	if(status !=B_OK) {
		return status;
	}
	status = message->FindInt64("node", &nodeRef.node);
	if(status !=B_OK) {
		return status;
	}
	
	status = watch_node(&nodeRef, B_STOP_WATCHING, this);
	if(status==B_OK)
		iWatchCount -=1;
	else
		return status;
		
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

	int32 op;
	entry_ref toEntryRef, fromEntryRef; 
	const char *name;
	
	message->FindInt32("opcode", &op);
	message->FindInt32("device", &fromEntryRef.device);

	
	if(op==B_ENTRY_MOVED)
	{
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
			for (auto const *x : fHandlers)
				x->OnStatChanged(&fromEntryRef);
			// iTreeView->OnStatChanged(&fromEntryRef);
			break;
		}
		case B_ENTRY_CREATED:
		{
			for (auto const *x : fHandlers)
				x->OnCreated(&fromEntryRef);
			// iTreeView->OnCreated(&fromEntryRef);
			break;
		}
		case B_ENTRY_REMOVED:
		{
			for (auto const *x : fHandlers)
				x->OnRemoved(&fromEntryRef);
			// iTreeView->OnRemoved(&fromEntryRef); 
			break;
		}
		case B_ENTRY_MOVED:
		{
			for (auto const *x : fHandlers)
				x->OnMoved(&fromEntryRef, &toEntryRef);
			// iTreeView->OnMoved(&fromEntryRef, &toEntryRef);
			break;
		}
		default:
			break;
	}
}

status_t 
NodeMonitor::AddHandler(INodeMonitorHandler *handler)
{
	if (handler) {
		fHandlers.emplace_back(handler);
	} else {
		return B_ERROR;
	}
	return B_OK;
}

status_t 
NodeMonitor::RemoveHandler(INodeMonitorHandler *handler)
{
	if (handler) {
		fHandlers.remove(handler);
	} else {
		return B_ERROR;
	}
	return B_OK;
}