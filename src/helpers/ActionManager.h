/*
 * Copyright 2023-2024, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <MenuItem.h>
#include <String.h>

#include <map>


class Action;
class ToolBar;
class ActionManager {
public:
	static status_t RegisterAction(int32   msgWhat,
							BString label,
							BString toolTip = "",
							BString iconResource = "",
							char shortcut = 0, uint32 modifiers = 0);

	static BMenuItem*	CreateMenuItem(int32 msgWhat);

	static status_t		AddItem(int32 msgWhat, BMenu*, BMessage* extraFields = nullptr);
	static status_t		AddItem(int32 msgWhat, ToolBar*, BMessage* extraFields = nullptr);

	static status_t		SetEnabled(int32 msgWhat, bool enabled);
	static status_t		SetPressed(int32 msgWhat, bool pressed);

	static status_t		SetToolTip(int32 msgWhat, const char* tooltip);

	static bool			IsPressed(int32 msgWhat);
	static bool			IsEnabled(int32 msgWhat);

	// accessing specific BMessage
	static BMessage*	GetMessage(int32 msgWhat, BMenu*);
	//TODO: static BMessage*	GetMessage(int32 msgWhat, ToolBar*);

	// disable copy costructor and assignment operator
	ActionManager(const ActionManager &) = delete;
	ActionManager & operator = (const ActionManager &) = delete;

private:
	typedef std::map<int32, Action*> ActionMap;
	ActionMap	fActionMap;

	static ActionManager sInstance;

	 ActionManager() {};
	~ActionManager();
};
