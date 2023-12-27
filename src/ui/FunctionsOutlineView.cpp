/*
 * Copyright 2024, My Name <my@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FunctionsOutlineView.h"


FunctionsOutlineView::FunctionsOutlineView()
	:
	BOutlineListView("outline")
{
}

void
FunctionsOutlineView::UpdateDocumentSymbols(BMessage* msg)
{
	MakeEmpty();
	int32 i=0;
	BString name;
	while(msg->FindString("name", i++, &name) == B_OK) {
		AddItem(new BStringItem(name));
	}
}