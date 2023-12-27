/*
 * Copyright 2024, My Name <my@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FunctionsOutlineView.h"
#include <Window.h>

#define kGoToSymbol	'gots'

class SymbolListItem : public BStringItem {
public:
		SymbolListItem(BMessage& details)
			: BStringItem(details.GetString("name"))
			, fDetails ( details )
			{}

		const BMessage& Details(){ return fDetails; }
private:
		BMessage	fDetails;
};

FunctionsOutlineView::FunctionsOutlineView()
	:
	BOutlineListView("outline")
{
}

void
FunctionsOutlineView::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case kGoToSymbol: {
			int32 index = msg->GetInt32("index", -1);
			if (index > -1) {
				SymbolListItem* sym = dynamic_cast<SymbolListItem*>(ItemAt(index));
				if (sym) {
					BMessage go = sym->Details();
					go.what = B_REFS_RECEIVED;
					go.AddRef("refs", &fCurrentRef);
					Window()->PostMessage(&go);
				}
			}
		}
		break;
		default:
			BOutlineListView::MessageReceived(msg);
	};
}

void
FunctionsOutlineView::UpdateDocumentSymbols(BMessage* msg)
{
	MakeEmpty();

	if (msg->FindRef("ref", &fCurrentRef) != B_OK)
		return;

	_RecursiveAddSymbols(nullptr, msg);
	SetInvocationMessage(new BMessage(kGoToSymbol));
	SetTarget(this);
}

void
FunctionsOutlineView::_RecursiveAddSymbols(BListItem* parent, BMessage* msg)
{
	int32 i=0;
	BMessage symbol;
	while(msg->FindMessage("symbol", i++, &symbol) == B_OK) {
		SymbolListItem* dad = new SymbolListItem(symbol);
		if (parent)
			AddUnder(dad, parent);
		else
			AddItem(dad);

		BMessage child;
		if (symbol.FindMessage("children", &child) == B_OK) {
			_RecursiveAddSymbols(dad, &child);
		}
	}
}