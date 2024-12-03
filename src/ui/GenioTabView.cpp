/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GenioTabView.h"
#include <LayoutBuilder.h>

GenioTabView::GenioTabView(BHandler* handler) : TabManager(BMessenger(this))
{
	font_height fh;
	GetFontHeight(&fh);
	float tabHeight = ceilf(fh.ascent + fh.descent + fh.leading + 8.0f);
	GetTabContainerView()->SetExplicitMaxSize(BSize(B_SIZE_UNSET, tabHeight));
	GetTabContainerView()->SetExplicitMinSize(BSize(B_SIZE_UNSET, tabHeight));
	SetDirtyFrameHack(TabGroup()->Frame());

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(TabGroup())
		.Add(ContainerView())
	.End();
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