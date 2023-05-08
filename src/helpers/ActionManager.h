/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ActionManager_H
#define ActionManager_H


#include <SupportDefs.h>
#include <map>
#include <String.h>
#include <MenuItem.h>
#include "ToolBar.h"

class Action;

class ActionManager {
public:

			~ActionManager();

	status_t RegisterAction(int32   msgWhat, 
							BString label, 
							BString toolTip,
							BString iconResource,
							char shortcut = 0, uint32 modifiers = 0);
	
	BMenuItem*	CreateMenuItem(int32 msgWhat);
	
	status_t	AddItem(int32 msgWhat, BMenu*);
	status_t    AddItem(int32 msgWhat, ToolBar*);
	
	status_t	SetEnabled(int32 msgWhat, bool enabled);

private:

	typedef std::map<int32, Action*> ActionMap;
	ActionMap	fActionMap;
};


#endif // ActionManager_H
