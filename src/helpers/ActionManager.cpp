/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ActionManager.h"
#include "ObjectList.h"

class Action {		
public:
	BString	fLabel;
	BString fIconResourceName;
	BString fToolTip;
	char	fShortcut;
	uint32  fModifiers;
	bool	fEnabled;
	bool	fPressed;
	
	BObjectList<BMenuItem>	fMenuItemList;
	BObjectList<ToolBar>	fToolBarList;
};

ActionManager::~ActionManager()
{
	ActionMap::reverse_iterator it;
	for (it = fActionMap.rbegin(); it != fActionMap.rend(); it++) {
		delete it->second;
	}
	fActionMap.clear();
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
	action->fEnabled = true; //by default?
	action->fPressed = false;
	action->fLabel = label;
	action->fIconResourceName = iconResource;
	action->fToolTip = toolTip;
	action->fShortcut = shortcut;
	action->fModifiers = modifiers;
	fActionMap[msgWhat] = action;
	return B_OK;
}

status_t
ActionManager::AddItem(int32 msgWhat, BMenu* menu)
{
	if (fActionMap.find(msgWhat) == fActionMap.end())
		return B_ERROR;
	
	Action* action = fActionMap[msgWhat];
	BMenuItem* item = new BMenuItem(action->fLabel, new BMessage(msgWhat), action->fShortcut, action->fModifiers);
	menu->AddItem(item);
	item->SetEnabled(action->fEnabled);
	item->SetMarked(action->fPressed);
	action->fMenuItemList.AddItem(item);
	return B_OK;
}

status_t
ActionManager::AddItem(int32 msgWhat, ToolBar* bar)
{
	if (fActionMap.find(msgWhat) == fActionMap.end())
		return B_ERROR;
	Action* action = fActionMap[msgWhat];
	bar->AddAction(msgWhat, action->fToolTip, action->fIconResourceName);
	bar->SetActionEnabled(msgWhat, action->fEnabled);
	bar->SetActionPressed(msgWhat, action->fPressed);
	action->fToolBarList.AddItem(bar);
	return B_OK;
}

status_t	
ActionManager::SetEnabled(int32 msgWhat, bool enabled)
{
	if (fActionMap.find(msgWhat) == fActionMap.end())
		return B_ERROR;
	Action* action = fActionMap[msgWhat];
	
	action->fEnabled = enabled;
	
	for (int i=0; i<action->fMenuItemList.CountItems();i++)
		action->fMenuItemList.ItemAt(i)->SetEnabled(enabled);
	for (int i=0; i<action->fToolBarList.CountItems();i++)
		action->fToolBarList.ItemAt(i)->SetActionEnabled(msgWhat, enabled);
	
	return B_OK;
	
}

status_t	
ActionManager::SetPressed(int32 msgWhat, bool pressed)
{
	if (fActionMap.find(msgWhat) == fActionMap.end())
		return B_ERROR;
	Action* action = fActionMap[msgWhat];
	
	action->fPressed = pressed;
	
	for (int i=0; i<action->fMenuItemList.CountItems();i++)
		action->fMenuItemList.ItemAt(i)->SetMarked(pressed);
		
	for (int i=0; i<action->fToolBarList.CountItems();i++)
		action->fToolBarList.ItemAt(i)->SetActionPressed(msgWhat, pressed);
	
	return B_OK;
	
}

bool		
ActionManager::IsPressed(int32 msgWhat)
{
	if (fActionMap.find(msgWhat) == fActionMap.end())
		return false;
	Action* action = fActionMap[msgWhat];
	
	return action->fPressed;
}