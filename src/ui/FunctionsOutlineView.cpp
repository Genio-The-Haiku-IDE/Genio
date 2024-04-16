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
#include "EditorMessages.h"
#include "EditorTabManager.h"
#include "GenioWindow.h"
#include "GenioWindowMessages.h"
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

		virtual void DrawItem(BView* owner, BRect bounds, bool complete);
		const BMessage& Details() const { return fDetails; }
private:
		BMessage	fDetails;
};


/* virtual */
void
SymbolListItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
#if 1
	BStringItem::DrawItem(owner, bounds, complete);
#else
	// TODO: just an example
	BString text(Text());
	BString newText;
	BString prefix;
	int32 kind;
	fDetails.FindInt32("kind", &kind);
	switch (kind) {
		case 2:
			// method
			prefix = "\xe2\x8c\xab";
			break;
		case 3:
			// function
			prefix = "\xe2\x8f\xb9";
			break;
		case 5:
			// field
			prefix = "\xe2\x8c\xac";
			break;
		case 6:
			// Variable
			prefix = "\xe2\x8c\xab";
			break;
		case 7:
			// Class
			prefix = "\xe2\x8c\xac";
			break;
		default:
			break;
	}

	if (!prefix.IsEmpty())
		newText.Append(prefix).Append(" ");
	newText.Append(text);
	SetText(newText.String());
	BStringItem::DrawItem(owner, bounds, complete);
	SetText(text.String());
#endif
}


// Sort by line
static int
CompareItems(const BListItem* itemA, const BListItem* itemB)
{
	const SymbolListItem* A = static_cast<const SymbolListItem*>(itemA);
	const SymbolListItem* B = static_cast<const SymbolListItem*>(itemB);

	int32 lineA;
	A->Details().FindInt32("be:line", &lineA);

	int32 lineB;
	B->Details().FindInt32("be:line", &lineB);

	return lineA - lineB;
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
	// TODO: other actions
	fToolBar->AddAction(kMsgSort, B_TRANSLATE("Sort"), "kIconOutlineSort", true);
	fToolBar->SetExplicitMinSize(BSize(250, B_SIZE_UNSET));
	fToolBar->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, 0)
			.Add(fToolBar)
			.Add(fScrollView)
		.End();
}


/* virtual */
void
FunctionsOutlineView::AttachedToWindow()
{
	BView::AttachedToWindow();
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
	BView::DetachedFromWindow();
}


/* virtual */
void
FunctionsOutlineView::Pulse()
{
	Editor* editor = gMainWindow->TabManager()->SelectedEditor();
	if (editor == nullptr)
		return;

	// TODO: Not very nice: we should request symbols update only if there's a
	// change. But we shouldn't ask one for EVERY change.
	// Maybe editor knows, but I wasn't able to make it work there

	// Update every 5 seconds
	const bigtime_t kUpdatePeriod = 5000000LL;

	bigtime_t currentTime = system_time();
	if (currentTime - fSymbolsLastUpdateTime > kUpdatePeriod) {
		// Window will request symbols through Editor
		Window()->PostMessage(kClassOutline);

		fStatus = STATUS_PENDING;
		// If list is empty, add "Pending..."
		if (fListView->CountItems() == 1) {
			fListView->MakeEmpty();
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
					LogTrace("FunctionsOutlineView: Symbols updated message received");
					entry_ref newRef;
					if (msg->FindRef("ref", &newRef) != B_OK) {
						// No ref means we don't have any opened file
						fListView->MakeEmpty();
						fListView->AddItem(new BStringItem(B_TRANSLATE("Empty")));
						fStatus = STATUS_EMPTY;
						break;
					}
					bool pending = false;
					msg->FindBool("pending", &pending);
					BMessage symbols;
					msg->FindMessage("symbols", &symbols);
					symbols.PrintToStream();
					_UpdateDocumentSymbols(symbols, &newRef, pending);
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
FunctionsOutlineView::_UpdateDocumentSymbols(const BMessage& msg,
	const entry_ref* newRef, bool pending)
{
	LogTrace("FunctionsOutlineView::_UpdateDocumentSymbol()");

	BStringItem* selected = dynamic_cast<BStringItem*>(fListView->ItemAt(fListView->CurrentSelection()));
	BString selectedItemText;
	if (selected != nullptr)
		selectedItemText = selected->Text();

	// Save the vertical scrolling value
	BScrollBar* vertScrollBar = fScrollView->ScrollBar(B_VERTICAL);
	float scrolledValue = vertScrollBar->Value();

	Editor* editor = gMainWindow->TabManager()->SelectedEditor();
	// Got a message from an unselected editor: ignore.
	if (editor != nullptr && *editor->FileRef() != *newRef) {
		LogTrace("Outline view got a message from an unselected editor. Ignoring...");
		return;
	}

	if (*newRef != fCurrentRef || fStatus == STATUS_EMPTY) {
		if (!msg.HasMessage("symbol") || pending) {
			// We could get a message with no symbols as soon as we switch tab
			// that means the LSP engine hasn't finished yet
			LogTrace("message without symbol messages");
			fListView->MakeEmpty();
			fListView->AddItem(new BStringItem(B_TRANSLATE("Pending" B_UTF8_ELLIPSIS)));
			fStatus = STATUS_PENDING;

			// TODO: We should request symbols to LSP, though
			return;
		}
	}

	fListView->MakeEmpty();
	_RecursiveAddSymbols(nullptr, &msg);
	fSymbolsLastUpdateTime = system_time();

	fStatus = STATUS_DONE;

	// same document, don't reset the vertical scrolling value
	if (*newRef == fCurrentRef) {
		vertScrollBar->SetValue(scrolledValue);
	}

	if (sSorted)
		fListView->SortItemsUnder(nullptr, true, &CompareItemsText);

	// List could have been changed.
	// Try to re-select old selected item, but only if it's the same document
	if (*newRef == fCurrentRef && !selectedItemText.IsEmpty()) {
		for (int32 i = 0; i < fListView->CountItems(); i++) {
			BStringItem* item = dynamic_cast<BStringItem*>(fListView->ItemAt(i));
			if (item != nullptr && selectedItemText == item->Text()) {
				fListView->Select(i, false);
				break;
			}
		}
	}

	fCurrentRef = *newRef;

	fListView->SetInvocationMessage(new BMessage(kGoToSymbol));
	fListView->SetTarget(this);
}


void
FunctionsOutlineView::_RecursiveAddSymbols(BListItem* parent, const BMessage* msg)
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
