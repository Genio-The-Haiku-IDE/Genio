/*
 * Original code taken from Seeker. Copyright by DarkWyrm
 * Copyright 2023 Nexus6 (nexus6.haiku@icloud.com)
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#ifndef _PNODEMONITOR_H_
#define _PNODEMONITOR_H_

#include <Handler.h>
#include <Entry.h>

#include <list>

struct INodeMonitorHandler
{
public:
	virtual void OnCreated(entry_ref *ref) const = 0;
	virtual void OnRemoved(entry_ref *ref) const = 0;
	virtual void OnMoved(entry_ref *origin, entry_ref *destination) const = 0;
	virtual void OnStatChanged(entry_ref *ref) const = 0;
	// virtual inline ~INodeMonitorHandler() {};
};

class NodeMonitor : public BHandler
{
public:
					NodeMonitor();
					~NodeMonitor();

	status_t		AddWatch(BMessage *message);
	status_t		RemoveWatch(BMessage *message);
	
	int32 			WatchCount() {return iWatchCount;}
	
	status_t		AddHandler(INodeMonitorHandler *handler);
	status_t		RemoveHandler(INodeMonitorHandler *handler);

protected:

	/* message handler */
	virtual void	MessageReceived(BMessage *message=NULL);		

private:

	std::list<INodeMonitorHandler*>	fHandlers;
	int32			iWatchCount;		
};

#endif
