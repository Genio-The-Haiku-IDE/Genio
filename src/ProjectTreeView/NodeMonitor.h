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
};

class NodeMonitor : public BHandler
{
public:
					NodeMonitor();
					~NodeMonitor();

	status_t		AddMonitorHandler(INodeMonitorHandler *handler);
	status_t		RemoveMonitorHandler(INodeMonitorHandler *handler);
	
	status_t		StartWatching(const char* path);
	status_t		StopWatching(const char* path);

	void			MessageReceived(BMessage *message) override;		

private:

	std::list<INodeMonitorHandler*>	fHandlers;
};

#endif
