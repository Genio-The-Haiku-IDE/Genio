/*
 * Copyright 2024, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FunctionsOutlineView.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <NaturalCompare.h>
#include <OutlineListView.h>
#include <StringView.h>
#include <Window.h>

#include "Editor.h"
#include "EditorMessages.h"
#include "ToolBar.h"

#define kGoToSymbol	'gots'
#define kMsgSort 'sort'
#define kMsgCollapseAll 'coll'

static bool sSorted = false;
static bool sCollapsed = false;

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


static int
CompareItems(const BListItem* itemA, const BListItem* itemB)
{
	return itemA - itemB;
}


static int
CompareItemsText(const BListItem* itemA, const BListItem* itemB)
{
	return BPrivate::NaturalCompare(((BStringItem*)itemA)->Text(), ((BStringItem*)itemB)->Text());
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FunctionsOutlineView"


FunctionsOutlineView::FunctionsOutlineView()
	:
	BView(B_TRANSLATE("Class outline"), B_WILL_DRAW),
	fListView(nullptr),
	fToolBar(nullptr),
	fLastUpdateTime(system_time())
{
	SetFlags(Flags() | B_PULSE_NEEDED);

	fListView = new BOutlineListView("listview");
	BScrollView* scrollView = new BScrollView("scrollview", fListView,
		B_FRAME_EVENTS | B_WILL_DRAW, true, true, B_FANCY_BORDER);
	fToolBar = new ToolBar();
	fToolBar->ChangeIconSize(16);
	// TODO: Icons
	//fToolBar->AddAction(kMsgCollapseAll, "Collapse all", "kIconGitRepo", true);
	fToolBar->AddAction(kMsgSort, B_TRANSLATE("Sort"), "kIconGitMore", true);
	fToolBar->SetExplicitMinSize(BSize(250, B_SIZE_UNSET));
	fToolBar->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 0)
			.Add(fToolBar)
			.Add(scrollView)
		.End();
}


/* virtual */
void
FunctionsOutlineView::AttachedToWindow()
{
	fToolBar->SetTarget(this);
	if (LockLooper()) {
		Window()->StartWatching(this, EDITOR_UPDATE_SYMBOLS);
		UnlockLooper();
	}
}


/* virtual */
void
FunctionsOutlineView::DetachedFromWindow()
{
	if (LockLooper()) {
		Window()->StopWatching(this, EDITOR_UPDATE_SYMBOLS);
		UnlockLooper();
	}
}


/* virtual */
void
FunctionsOutlineView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case EDITOR_UPDATE_SYMBOLS:
				{
					BMessage symbols;
					msg->FindMessage("symbols", &symbols);
					_UpdateDocumentSymbols(&symbols);
					break;
				}
				default:
					break;
			}
			break;
		}
		case kGoToSymbol:
		{
			int32 index = msg->GetInt32("index", -1);
			if (index > -1) {
				SymbolListItem* sym = dynamic_cast<SymbolListItem*>(fListView->ItemAt(index));
				if (sym != nullptr) {
					BMessage go = sym->Details();
					go.what = B_REFS_RECEIVED;
					go.AddRef("refs", &fCurrentRef);
					Window()->PostMessage(&go);
				}
			}
			break;
		}
		case kMsgSort:
		{
			sSorted = !sSorted;
			fToolBar->SetActionPressed(kMsgSort, sSorted);
			if (sSorted)
				fListView->SortItemsUnder(nullptr, true, &CompareItemsText);
			else
				fListView->SortItemsUnder(nullptr, true, &CompareItems);
			break;
		}
		case kMsgCollapseAll:
		{
			sCollapsed = !sCollapsed;
			fToolBar->SetActionPressed(kMsgSort, sCollapsed);
			if (sCollapsed)
				fListView->Collapse(nullptr);
			else
				fListView->Expand(nullptr);
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


/* virtual */
void
FunctionsOutlineView::Pulse()
{
	// Update every 10 seconds
	const bigtime_t kUpdatePeriod = 10000000LL;

	bigtime_t currentTime = system_time();
	if (currentTime - fLastUpdateTime > kUpdatePeriod) {
		// TODO: Maybe move this to the Editor which knows if a file has been modified
		// TODO: we send a message to the window which sends it to the editor
		// which then ... : refactor
		Window()->PostMessage(kClassOutline);
		fLastUpdateTime = currentTime;
	}
}


void
FunctionsOutlineView::_UpdateDocumentSymbols(BMessage* msg)
{
	BStringItem* selected = dynamic_cast<BStringItem*>(fListView->ItemAt(fListView->CurrentSelection()));
	BString selectedItemText;
	if (selected != nullptr)
		selectedItemText = selected->Text();

	fListView->MakeEmpty();
	// TODO: Improve
	fListView->AddItem(new BStringItem(B_TRANSLATE("Pending" B_UTF8_ELLIPSIS)));
	if (msg->FindRef("ref", &fCurrentRef) != B_OK)
		return;

	fListView->MakeEmpty();
	// TODO: This is done synchronously
	_RecursiveAddSymbols(nullptr, msg);

	// List could have been changed. Try to re-select old selected item
	if (!selectedItemText.IsEmpty()) {
		for (int32 i = 0; i < fListView->CountItems(); i++) {
			BStringItem* item = dynamic_cast<BStringItem*>(fListView->ItemAt(i));
			if (item != nullptr && selectedItemText == item->Text()) {
				fListView->Select(i, false);
				break;
			}
		}
	}

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
		if (parent != nullptr)
			fListView->AddUnder(dad, parent);
		else
			fListView->AddItem(dad);

		BMessage child;
		if (symbol.FindMessage("children", &child) == B_OK) {
			_RecursiveAddSymbols(dad, &child);
		}
	}
}
