/*
 * Copyright 2023-2024, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ActionManager.h"

#include <Button.h>
#include <vector>

#include "ObjectList.h"
#include "ToolBar.h"

ActionManager ActionManager::sInstance;

class Action {
public:
	BString	label;
	BString iconResourceName;
	BString toolTip;
	uint32  modifiers;
	char	shortcut;
	bool	enabled;
	bool	pressed;

	std::vector<BMenuItem*>	menuItemList;
	std::vector<ToolBar*>	toolBarList;
};


ActionManager::~ActionManager()
{
	printf("sizeof action: %ld\n", sizeof(Action));
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
	action->menuItemList.push_back(item);
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
	action->toolBarList.push_back(bar);
	return B_OK;
}


status_t
ActionManager::SetEnabled(int32 msgWhat, bool enabled)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return B_ERROR;

	Action* action = sInstance.fActionMap[msgWhat];
	action->enabled = enabled;

	for (auto menuItem : action->menuItemList)
		menuItem->SetEnabled(enabled);
	for (auto toolBar : action->toolBarList)
		toolBar->SetActionEnabled(msgWhat, enabled);

	return B_OK;
}


status_t
ActionManager::SetPressed(int32 msgWhat, bool pressed)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return B_ERROR;

	Action* action = sInstance.fActionMap[msgWhat];
	action->pressed = pressed;

	for (auto menuItem : action->menuItemList)
		menuItem->SetMarked(pressed);
	for (auto toolBar : action->toolBarList)
		toolBar->SetActionPressed(msgWhat, pressed);

	return B_OK;

}


status_t
ActionManager::SetToolTip(int32 msgWhat, const char* tooltip)
{
	if (sInstance.fActionMap.find(msgWhat) == sInstance.fActionMap.end())
		return B_ERROR;

	Action* action = sInstance.fActionMap[msgWhat];
	action->toolTip = tooltip;

	for (auto toolBar : action->toolBarList) {
		BButton* button = toolBar->FindButton(msgWhat);
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
	for (auto menuItem : action->menuItemList)
		if (menuItem->Menu() == menu)
			return menuItem->Message();

	return nullptr;
}
