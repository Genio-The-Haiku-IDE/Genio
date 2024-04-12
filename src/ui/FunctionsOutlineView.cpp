/*
 * Copyright 2024, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FunctionsOutlineView.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <NaturalCompare.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Window.h>

#include "Editor.h"
#include "EditorTabManager.h"
#include "GenioWindow.h"
#include "GenioWindowMessages.h"
#include "LSPEditorWrapper.h"
#include "ToolBar.h"

#include "Log.h"

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
	fStatus(STATUS_EMPTY),
	fSymbolsLastUpdateTime(0)
{
	SetFlags(Flags() | B_PULSE_NEEDED);

	fListView = new BOutlineListView("listview");
	fScrollView = new BScrollView("scrollview", fListView,
		B_FRAME_EVENTS | B_WILL_DRAW, true, true, B_FANCY_BORDER);
	fToolBar = new ToolBar();
	fToolBar->ChangeIconSize(16);
	// TODO: Icons
	//fToolBar->AddAction(kMsgCollapseAll, "Collapse all", "kIconGitRepo", true);
	fToolBar->AddAction(kMsgSort, B_TRANSLATE("Sort"), "kIconOutlineSort", true);
	fToolBar->SetExplicitMinSize(BSize(250, B_SIZE_UNSET));
	fToolBar->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 0)
			.Add(fToolBar)
			.Add(fScrollView)
		.End();

	_UpdateDocumentSymbols(nullptr);
}


/* virtual */
void
FunctionsOutlineView::AttachedToWindow()
{
	fToolBar->SetTarget(this);
	if (LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED);
		UnlockLooper();
	}
}


/* virtual */
void
FunctionsOutlineView::DetachedFromWindow()
{
	if (LockLooper()) {
		Window()->StopWatching(this, MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED);
		UnlockLooper();
	}
}


/* virtual */
void
FunctionsOutlineView::Pulse()
{
	Editor* editor = gMainWindow->TabManager()->SelectedEditor();
	// We already requested symbols, just need to wait a bit
	if (editor != nullptr && *editor->FileRef() == fCurrentRef &&
			fStatus == STATUS_PENDING)
		return;

	// TODO: Not very nice: we should request symbols update only if there's a
	// change. But we shouldn't ask one for EVERY change.
	// Maybe editor knows, but I wasn't able to make it work there
	// Update every 10 seconds
	const bigtime_t kUpdatePeriod = 5000000LL;

	// TODO: Not very nice design-wise, either:
	// ideally we shouldn't know about Editor or LSPEditorWrapper.
	// There should be a way for Editor to retrieving symbols when needed
	// and then we would receive this info via BMessage
	bigtime_t currentTime = system_time();
	if (currentTime - fSymbolsLastUpdateTime > kUpdatePeriod) {
		editor->GetLSPEditorWrapper()->RequestDocumentSymbols();
		fStatus = STATUS_PENDING;
		// If list is empty, add "Pending..."
		if (fListView->CountItems() == 0) {
			LogTrace("_UpdateDocumentSymbol(): found ref.");
			// Add "pending..."
			// TODO: Improve
			fListView->AddItem(new BStringItem(B_TRANSLATE("Pending" B_UTF8_ELLIPSIS)));
		}
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
				case MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED:
				{
					fSymbolsLastUpdateTime = system_time();
					BMessage symbols;
					if (msg->FindMessage("symbols", &symbols) == B_OK) {
						_UpdateDocumentSymbols(&symbols);
					} else
						_UpdateDocumentSymbols(nullptr);
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


void
FunctionsOutlineView::_UpdateDocumentSymbols(BMessage* msg)
{
	LogTrace("_UpdateDocumentSymbol()");
	if (msg == nullptr) {
		fListView->MakeEmpty();
		return;
	}

	BStringItem* selected = dynamic_cast<BStringItem*>(fListView->ItemAt(fListView->CurrentSelection()));
	BString selectedItemText;
	if (selected != nullptr)
		selectedItemText = selected->Text();

	// Save the vertical scrolling value
	BScrollBar* vertScrollBar = fScrollView->ScrollBar(B_VERTICAL);
	float scrolledValue = vertScrollBar->Value();

	entry_ref newRef;
	if (msg->FindRef("ref", &newRef) != B_OK) {
		LogTrace("_UpdateDocumentSymbol(): ref not found.");
		fListView->MakeEmpty();
		return;
	}

	LogTrace("_UpdateDocumentSymbol(): found ref.");

	fListView->MakeEmpty();
	// TODO: This is done synchronously
	_RecursiveAddSymbols(nullptr, msg);

	fStatus = STATUS_LOADED;

	// same document, don't reset the vertical scrolling value
	if (newRef == fCurrentRef) {
		vertScrollBar->SetValue(scrolledValue);
	}

	if (sSorted)
		fListView->SortItemsUnder(nullptr, true, &CompareItemsText);

	// List could have been changed.
	// Try to re-select old selected item, but only if it's the same document
	if (newRef == fCurrentRef && !selectedItemText.IsEmpty()) {
		for (int32 i = 0; i < fListView->CountItems(); i++) {
			BStringItem* item = dynamic_cast<BStringItem*>(fListView->ItemAt(i));
			if (item != nullptr && selectedItemText == item->Text()) {
				fListView->Select(i, false);
				break;
			}
		}
	}

	fCurrentRef = newRef;

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
