/*
 * Copyright 2023-2024, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ActionManager.h"

#include <Button.h>

#include "ObjectList.h"
#include "ToolBar.h"

ActionManager ActionManager::sInstance;

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
	for (it = sInstance.fActionMap.rbegin(); it != sInstance.fActionMap.rend(); it++) {
		delete it->second;
	}
	sInstance.fActionMap.clear();
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
	sInstance.fActionMap[msgWhat] = action;
	return B_OK;
}


status_t
ActionManager::AddItem(int32 msgWhat, BMenu* menu, BMessage* extraFields)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return B_ERROR;

	Action* action = sInstance.fActionMap[msgWhat];
	if (extraFields == nullptr) {
		extraFields = new BMessage(msgWhat);
	}
	extraFields->what = msgWhat;
	BMenuItem* item = new BMenuItem(action->label, extraFields, action->shortcut, action->modifiers);
	menu->AddItem(item);
	item->SetEnabled(action->enabled);
	item->SetMarked(action->pressed);
	action->menuItemList.AddItem(item);
	return B_OK;
}


status_t
ActionManager::AddItem(int32 msgWhat, ToolBar* bar, BMessage* extraFields)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return B_ERROR;

	Action* action = sInstance.fActionMap[msgWhat];
	if (extraFields == nullptr) {
		extraFields = new BMessage(msgWhat);
	}
	extraFields->what = msgWhat;
	bar->AddAction(extraFields, action->toolTip, action->iconResourceName, true);
	bar->SetActionEnabled(msgWhat, action->enabled);
	bar->SetActionPressed(msgWhat, action->pressed);
	action->toolBarList.AddItem(bar);
	return B_OK;
}


status_t
ActionManager::SetEnabled(int32 msgWhat, bool enabled)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return B_ERROR;

	Action* action = sInstance.fActionMap[msgWhat];
	action->enabled = enabled;

	for (int32 i = 0; i < action->menuItemList.CountItems(); i++)
		action->menuItemList.ItemAt(i)->SetEnabled(enabled);
	for (int32 i = 0; i < action->toolBarList.CountItems(); i++)
		action->toolBarList.ItemAt(i)->SetActionEnabled(msgWhat, enabled);

	return B_OK;
}


status_t
ActionManager::SetPressed(int32 msgWhat, bool pressed)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return B_ERROR;

	Action* action = sInstance.fActionMap[msgWhat];
	action->pressed = pressed;

	for (int32 i = 0; i < action->menuItemList.CountItems(); i++)
		action->menuItemList.ItemAt(i)->SetMarked(pressed);

	for (int32 i = 0; i < action->toolBarList.CountItems(); i++)
		action->toolBarList.ItemAt(i)->SetActionPressed(msgWhat, pressed);

	return B_OK;

}


status_t
ActionManager::SetToolTip(int32 msgWhat, const char* tooltip)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return B_ERROR;

	Action* action = sInstance.fActionMap[msgWhat];
	action->toolTip = tooltip;

	for (int32 i = 0; i < action->toolBarList.CountItems(); i++) {
		BButton* button = action->toolBarList.ItemAt(i)->FindButton(msgWhat);
		if (button)
			button->SetToolTip(action->toolTip);
	}

	return B_OK;
}


bool
ActionManager::IsPressed(int32 msgWhat)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return false;

	Action* action = sInstance.fActionMap[msgWhat];
	return action->pressed;
}


bool
ActionManager::IsEnabled(int32 msgWhat)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return false;

	Action* action = sInstance.fActionMap[msgWhat];
	return action->enabled;
}


/*static*/
BMessage*
ActionManager::GetMessage(int32 msgWhat, BMenu* menu)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return nullptr;

	Action* action = sInstance.fActionMap[msgWhat];
	for (int32 i = 0; i < action->menuItemList.CountItems(); i++)
		if (action->menuItemList.ItemAt(i)->Menu() == menu)
			return action->menuItemList.ItemAt(i)->Message();

	return nullptr;
}
