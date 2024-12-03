/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GenioTabView.h"
#include <LayoutBuilder.h>
#include <ControlLook.h>

GenioTabView::GenioTabView(BHandler* handler) : TabManager(BMessenger(this))
{
	GetTabContainerView()->SetExplicitMaxSize(BSize(B_SIZE_UNSET, TabHeight()));
	GetTabContainerView()->SetExplicitMinSize(BSize(B_SIZE_UNSET, TabHeight()));
	SetDirtyFrameHack(TabGroup()->Frame());

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(TabGroup())
		.Add(ContainerView())
	.End();

	// BSize minSize = GetTabContainerView()->PreferredSize();
	// minSize.height = 500;
	// SetExplicitMinSize(minSize);
}


float
GenioTabView::TabHeight() const
{
	font_height fh;
	GetFontHeight(&fh);
	return ceilf(fh.ascent + fh.descent + fh.leading +
		(be_control_look->DefaultLabelSpacing() * 1.3f));

}

void
GenioTabView::AttachedToWindow()
{
	SetTarget(this);
}


void
GenioTabView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case TABMANAGER_TAB_SELECTED:
			//debugger("ok");

		break;
		case TABMANAGER_TAB_CLOSE_MULTI:
		{
			type_code typeFound;
			int32 countFound;
			bool fixedSize;
			if (message->GetInfo("index", &typeFound, &countFound, &fixedSize) == B_OK) {
				int32 index;
				for (int32 i = 0; i < countFound; i++) {
					if (message->FindInt32("index", i, &index) == B_OK) {
						RemoveTab(index);
						//HideTab(index);
					}
				}
			}
		}
		break;
		case TABMANAGER_TAB_NEW_OPENED:

		break;
		default:
			BGroupView::MessageReceived(message);
		break;
	};
}


void
GenioTabView::AddTab(tab_key id, BView* view)
{
	fTabMap[id] = view;
	TabManager::AddTab(view, view->Name(), CountTabs());
	TabManager::SelectTab(0); //workaround..
}


void
GenioTabView::SetTabLabel(BView* view, const char* label)
{
	int32 index = TabForView(view);
	if (index >= 0)
		TabManager::SetTabLabel(index, label);
}


void
GenioTabView::SelectTab(tab_key id)
{
	if (fTabMap.contains(id)) {
		BView* view = fTabMap[id];
		if (view->Parent() == nullptr)
			return;
		int32 index = TabForView(view);
		if (index >= 0)
			TabManager::SelectTab(index);
	}
}