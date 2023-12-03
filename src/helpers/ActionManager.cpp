/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ActionManager.h"
#include "ObjectList.h"
#include <Button.h>

ActionManager ActionManager::instance;

class Action {
public:
	BString	label;
	BString iconResourceName;
	BString toolTip;
	char	shortcut;
	uint32  modifiers;
	bool	enabled;
	bool	pressed;

	BObjectList<BMenuItem>	menuItemList;
	BObjectList<ToolBar>	toolBarList;
};

ActionManager::~ActionManager()
{
	ActionMap::reverse_iterator it;
	for (it = instance.fActionMap.rbegin(); it != instance.fActionMap.rend(); it++) {
		delete it->second;
	}
	instance.fActionMap.clear();
}

status_t
ActionManager::RegisterAction(int32   msgWhat,
								BString label,
								BString toolTip,
								BString iconResource,
								char shortcut,
								uint32 modifiers)
{
	Action* action = new Action();
	action->enabled = true; //by default?
	action->pressed = false;
	action->label = label;
	action->iconResourceName = iconResource;
	action->toolTip = toolTip;
	action->shortcut = shortcut;
	action->modifiers = modifiers;
	instance.fActionMap[msgWhat] = action;
	return B_OK;
}

status_t
ActionManager::AddItem(int32 msgWhat, BMenu* menu)
{
	if (instance.fActionMap.find(msgWhat) == instance.fActionMap.end())
		return B_ERROR;

	Action* action = instance.fActionMap[msgWhat];
	BMenuItem* item = new BMenuItem(action->label, new BMessage(msgWhat), action->shortcut, action->modifiers);
	menu->AddItem(item);
	item->SetEnabled(action->enabled);
	item->SetMarked(action->pressed);
	action->menuItemList.AddItem(item);
	return B_OK;
}

status_t
ActionManager::AddItem(int32 msgWhat, ToolBar* bar)
{
	if (instance.fActionMap.find(msgWhat) == instance.fActionMap.end())
		return B_ERROR;
	Action* action = instance.fActionMap[msgWhat];
	bar->AddAction(msgWhat, action->toolTip, action->iconResourceName, true);
	bar->SetActionEnabled(msgWhat, action->enabled);
	bar->SetActionPressed(msgWhat, action->pressed);
	action->toolBarList.AddItem(bar);
	return B_OK;
}

status_t
ActionManager::SetEnabled(int32 msgWhat, bool enabled)
{
	if (instance.fActionMap.find(msgWhat) == instance.fActionMap.end())
		return B_ERROR;
	Action* action = instance.fActionMap[msgWhat];

	action->enabled = enabled;

	for (int i=0; i<action->menuItemList.CountItems();i++)
		action->menuItemList.ItemAt(i)->SetEnabled(enabled);
	for (int i=0; i<action->toolBarList.CountItems();i++)
		action->toolBarList.ItemAt(i)->SetActionEnabled(msgWhat, enabled);

	return B_OK;

}

status_t
ActionManager::SetPressed(int32 msgWhat, bool pressed)
{
	if (instance.fActionMap.find(msgWhat) == instance.fActionMap.end())
		return B_ERROR;
	Action* action = instance.fActionMap[msgWhat];

	action->pressed = pressed;

	for (int i=0; i<action->menuItemList.CountItems();i++)
		action->menuItemList.ItemAt(i)->SetMarked(pressed);

	for (int i=0; i<action->toolBarList.CountItems();i++)
		action->toolBarList.ItemAt(i)->SetActionPressed(msgWhat, pressed);

	return B_OK;

}

status_t
ActionManager::SetToolTip(int32 msgWhat, const char* tooltip)
{
	if (instance.fActionMap.find(msgWhat) == instance.fActionMap.end())
		return B_ERROR;

	Action* action = instance.fActionMap[msgWhat];

	action->toolTip = tooltip;

	for (int i=0; i<action->toolBarList.CountItems();i++) {
		BButton* button = action->toolBarList.ItemAt(i)->FindButton(msgWhat);
		if (button)
			button->SetToolTip(action->toolTip);
	}

	return B_OK;
}

bool
ActionManager::IsPressed(int32 msgWhat)
{
	if (instance.fActionMap.find(msgWhat) == instance.fActionMap.end())
		return false;
	Action* action = instance.fActionMap[msgWhat];

	return action->pressed;
}
bool
ActionManager::IsEnabled(int32 msgWhat)
{
	if (instance.fActionMap.find(msgWhat) == instance.fActionMap.end())
		return false;
	Action* action = instance.fActionMap[msgWhat];

	return action->enabled;
}