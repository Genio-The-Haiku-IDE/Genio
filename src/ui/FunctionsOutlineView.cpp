/*
 * Copyright 2024, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FunctionsOutlineView.h"

#include <LayoutBuilder.h>
#include <OutlineListView.h>
#include <StringView.h>
#include <Window.h>

#include "EditorMessages.h"

#define kGoToSymbol	'gots'

class SymbolListItem : public BStringItem {
public:
		SymbolListItem(BMessage& details)
			: BStringItem(details.GetString("name"))
			, fDetails ( details )
			{}

		const BMessage& Details() const { return fDetails; }
private:
		BMessage	fDetails;
};


FunctionsOutlineView::FunctionsOutlineView()
	:
	BView("outline", B_WILL_DRAW),
	fListView(nullptr),
	fLoadingView(nullptr),
	fLoadingStatus(STATUS_EMPTY),
	fLastUpdateTime(system_time())
{
	SetFlags(Flags() | B_PULSE_NEEDED);

	fListView = new BOutlineListView("listview");
	// TODO: Translate, improve.
	// TODO: should write "empty" at the center of the view, it does at bottom instead
	fLoadingView = new BStringView("status view", "empty");
	fLoadingView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fLoadingView->SetExplicitMinSize(BSize(B_SIZE_UNSET, B_SIZE_UNSET));
	fLoadingView->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_TOP));
	fLoadingView->SetAlignment(B_ALIGN_CENTER);

	BLayoutBuilder::Cards<>(this)
		.Add(fListView)
		.Add(fLoadingView)
		.SetVisibleItem(0)
		.End();
}


/* virtual */
void
FunctionsOutlineView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kGoToSymbol: {
			int32 index = msg->GetInt32("index", -1);
			if (index > -1) {
				SymbolListItem* sym = dynamic_cast<SymbolListItem*>(fListView->ItemAt(index));
				if (sym) {
					BMessage go = sym->Details();
					go.what = B_REFS_RECEIVED;
					go.AddRef("refs", &fCurrentRef);
					Window()->PostMessage(&go);
				}
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


/* virtual*/
void
FunctionsOutlineView::Pulse()
{
	// Update every 10 seconds
	bigtime_t currentTime = system_time();
	if (currentTime - fLastUpdateTime > 10000000LL) {
		// TODO: Maybe move this to the Editor which knows if a file has been modified
		// TODO: we send a message to the window which sends it to the editor
		// which then ... : refactor
		Window()->PostMessage(kClassOutline);
		fLastUpdateTime = currentTime;
	}
}


void
FunctionsOutlineView::SetLoadingStatus(status loadingStatus)
{
	fLoadingStatus = loadingStatus;
}


void
FunctionsOutlineView::UpdateDocumentSymbols(BMessage* msg)
{
	fListView->MakeEmpty();

	SetLoadingStatus(STATUS_EMPTY);
	dynamic_cast<BCardLayout*>(GetLayout())->SetVisibleItem(1);
	if (msg->FindRef("ref", &fCurrentRef) != B_OK)
		return;

	SetLoadingStatus(STATUS_LOADING);
	_RecursiveAddSymbols(nullptr, msg);
	SetLoadingStatus(STATUS_LOADED);
	dynamic_cast<BCardLayout*>(GetLayout())->SetVisibleItem(0);
	fListView->SetInvocationMessage(new BMessage(kGoToSymbol));
	fListView->SetTarget(this);
}


void
FunctionsOutlineView::_RecursiveAddSymbols(BListItem* parent, BMessage* msg)
{
	int32 i = 0;
	BMessage symbol;
	while (msg->FindMessage("symbol", i++, &symbol) == B_OK) {
		SymbolListItem* dad = new SymbolListItem(symbol);
		if (parent)
			fListView->AddUnder(dad, parent);
		else
			fListView->AddItem(dad);

		BMessage child;
		if (symbol.FindMessage("children", &child) == B_OK) {
			_RecursiveAddSymbols(dad, &child);
		}
	}
}
