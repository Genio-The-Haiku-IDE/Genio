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
#include "protocol_objects.h"
#include "StyledItem.h"
#include "ToolBar.h"

#include "Log.h"


#define kGoToSymbol	'gots'
#define kMsgSort 'sort'
#define kMsgCollapseAll 'coll'

static bool sSorted = false;
static bool sCollapsed = false;

class SymbolListItem : public StyledItem {
public:
		SymbolListItem(BMessage& details)
			:
			StyledItem(details.GetString("name")),
			fDetails(details)
		{
			SetIconFollowsTheme(true);
			SetIconAndTooltip();
		}

		virtual void DrawItem(BView* owner, BRect bounds, bool complete);
		const BMessage& Details() const { return fDetails; }
		void SetIconAndTooltip();
private:
		BMessage	fDetails;
};


/* virtual */
void
SymbolListItem::DrawItem(BView* owner, BRect bounds, bool complete)
{
	StyledItem::DrawItem(owner, bounds, complete);
}


void
SymbolListItem::SetIconAndTooltip()
{
	BString iconName;
	BString toolTip;
	int32 kind;
	Details().FindInt32("kind", &kind);
	SymbolKind symbolKind = static_cast<SymbolKind>(kind);

	switch (symbolKind) {
		case SymbolKind::File:
			iconName = "file";
			toolTip = "File";
			break;
		case SymbolKind::Module:
			iconName = "namespace";
			toolTip = "Module";
			break;
		case SymbolKind::Namespace:
			iconName = "namespace";
			toolTip = "Namespace";
			break;
		case SymbolKind::Package:
			iconName = "namespace";
			toolTip = "Package";
			break;
		case SymbolKind::Class:
			iconName = "class";
			toolTip = "Class";
			break;
		case SymbolKind::Method:
			iconName = "method";
			toolTip = "Method";
			break;
		case SymbolKind::Property:
			iconName = "property";
			toolTip = "Property";
			break;
		case SymbolKind::Field:
			iconName = "field";
			toolTip = "Field";
			break;
		case SymbolKind::Constructor:
			iconName = "method";
			toolTip = "Constructor";
			break;
		case SymbolKind::Enum:
			iconName = "enum";
			toolTip = "Enum";
			break;
		case SymbolKind::Interface:
			iconName = "interface";
			toolTip = "Interface";
			break;
		case SymbolKind::Function:
			iconName = "method";
			toolTip = "Function";
			break;
		case SymbolKind::Variable:
			iconName = "variable";
			toolTip = "Variable";
			break;
		case SymbolKind::Constant:
			iconName = "constant";
			toolTip = "Constant";
			break;
		case SymbolKind::String:
			iconName = "string";
			toolTip = "String";
			break;
		case SymbolKind::Number:
			iconName = "numeric";
			toolTip = "";
			break;
		case SymbolKind::Boolean:
			iconName = "boolean";
			toolTip = "Boolean";
			break;
		case SymbolKind::Array:
			iconName = "array";
			toolTip = "Array";
			break;
		case SymbolKind::Object:
			iconName = "namespace";
			toolTip = "Object";
			break;
		case SymbolKind::Key:
			iconName = "key";
			toolTip = "Key";
			break;
		case SymbolKind::Null: // icon not available
			iconName = "misc";
			toolTip = "Null";
			break;
		case SymbolKind::EnumMember:
			iconName = "emum-member";
			toolTip = "Enum member";
			break;
		case SymbolKind::Struct:
			iconName = "structure";
			toolTip = "Structure";
			break;
		case SymbolKind::Event:
			iconName = "event";
			toolTip = "Event";
			break;
		case SymbolKind::Operator:
			iconName = "operator";
			toolTip = "Operator";
			break;
		case SymbolKind::TypeParameter:
			iconName = "parameter";
			toolTip = "Type parameter";
			break;
		default:
			break;
	}

	if (!iconName.IsEmpty())
		SetIcon(iconName.Prepend("symbol-"));

	if (!toolTip.IsEmpty())
		SetToolTipText(toolTip);
}


// Sort by line
static int
CompareItems(const BListItem* itemA, const BListItem* itemB)
{
	const SymbolListItem* A = static_cast<const SymbolListItem*>(itemA);
	const SymbolListItem* B = static_cast<const SymbolListItem*>(itemB);

	int32 lineA;
	A->Details().FindInt32("start:line", &lineA);

	int32 lineB;
	B->Details().FindInt32("start:line", &lineB);

	return lineA - lineB;
}


static int
CompareItemsText(const BListItem* itemA, const BListItem* itemB)
{
	return BPrivate::NaturalCompare(((BStringItem*)itemA)->Text(), ((BStringItem*)itemB)->Text());
}


class SymbolOutlineListView: public BOutlineListView {
public:
	SymbolOutlineListView(const char* name) : BOutlineListView(name) {}

	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* message)
	{
		BOutlineListView::MouseMoved(point, transit, message);
		if ((transit == B_ENTERED_VIEW) || (transit == B_INSIDE_VIEW)) {
			auto index = IndexOf(point);
			if (index >= 0) {
				SymbolListItem *item = dynamic_cast<SymbolListItem*>(ItemAt(index));
				if (item->HasToolTip()) {
					SetToolTip(item->GetToolTipText());
				} else {
					SetToolTip("");
				}
			}
		}
	}
};


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FunctionsOutlineView"


FunctionsOutlineView::FunctionsOutlineView()
	:
	BView(B_TRANSLATE("Outline"), B_WILL_DRAW),
	fListView(nullptr),
	fToolBar(nullptr),
	fSymbolsLastUpdateTime(0)
{
	SetFlags(Flags() | B_PULSE_NEEDED);

	fListView = new SymbolOutlineListView("listview");
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
	BView::Pulse();

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
					BMessage symbols;
					if (msg->FindMessage("symbols", &symbols) != B_OK) {
						debugger("No symbols message");
						break;
					}

					LogTrace("FunctionsOutlineView: Symbols updated message received");
					entry_ref newRef;
					msg->FindRef("ref", &newRef);
					_UpdateDocumentSymbols(symbols, &newRef);
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
	const entry_ref* newRef)
{
	LogTrace("FunctionsOutlineView::_UpdateDocumentSymbol()");

	int32 status = Editor::STATUS_NOT_INITIALIZED;
	msg.FindInt32("status", &status);
	if (status == Editor::STATUS_NO_SYMBOLS) {
		// means we don't have any opened file, or file has no symbols
		fListView->MakeEmpty();
		fListView->AddItem(new BStringItem(B_TRANSLATE("No outline available")));
		return;
	}

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

	if (status == Editor::STATUS_NOT_INITIALIZED) {
		// File just opened, LSP hasn't retrieved symbols yet
		fListView->MakeEmpty();
		fListView->AddItem(new BStringItem(B_TRANSLATE("Creating outline" B_UTF8_ELLIPSIS)));

		// TODO: We should request symbols to LSP, though
		return;
	}

	fListView->MakeEmpty();
	_RecursiveAddSymbols(nullptr, &msg);
	fSymbolsLastUpdateTime = system_time();

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
		SymbolListItem* item = new SymbolListItem(symbol);
		if (parent != nullptr)
			fListView->AddUnder(item, parent);
		else
			fListView->AddItem(item);

		BMessage child;
		if (symbol.FindMessage("children", &child) == B_OK) {
			_RecursiveAddSymbols(item, &child);
		}
	}
}
