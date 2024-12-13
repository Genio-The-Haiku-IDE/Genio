/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GTab.h"

#include <GroupLayout.h>



GTabContainer::GTabContainer(BView* view):BBox(view->Name()),fView(view)
{
	SetBorder(B_NO_BORDER);
	SetLayout(new BGroupLayout(B_VERTICAL));
	if (fView)
		AddChild(fView);
}



GTab::GTab(BView* view, tab_id id):BTab(),fGTabContainer(nullptr),fTabId(id)
{
	fGTabContainer = new GTabContainer(view);
	SetView(fGTabContainer);
}



