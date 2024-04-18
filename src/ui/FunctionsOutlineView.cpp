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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "OutlineTooltips"

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
			toolTip = B_TRANSLATE("File");
			break;
		case SymbolKind::Module:
			iconName = "namespace";
			toolTip = B_TRANSLATE("Module");
			break;
		case SymbolKind::Namespace:
			iconName = "namespace";
			toolTip = B_TRANSLATE("Namespace");
			break;
		case SymbolKind::Package:
			iconName = "namespace";
			toolTip = B_TRANSLATE("Package");
			break;
		case SymbolKind::Class:
			iconName = "class";
			toolTip = B_TRANSLATE("Class");
			break;
		case SymbolKind::Method:
			iconName = "method";
			toolTip = B_TRANSLATE("Method");
			break;
		case SymbolKind::Property:
			iconName = "property";
			toolTip = B_TRANSLATE("Property");
			break;
		case SymbolKind::Field:
			iconName = "field";
			toolTip = B_TRANSLATE("Field");
			break;
		case SymbolKind::Constructor:
			iconName = "method";
			toolTip = B_TRANSLATE("Constructor");
			break;
		case SymbolKind::Enum:
			iconName = "enum";
			toolTip = B_TRANSLATE("Enum");
			break;
		case SymbolKind::Interface:
			iconName = "interface";
			toolTip = B_TRANSLATE("Interface");
			break;
		case SymbolKind::Function:
			iconName = "method";
			toolTip = B_TRANSLATE("Function");
			break;
		case SymbolKind::Variable:
			iconName = "variable";
			toolTip = B_TRANSLATE("Variable");
			break;
		case SymbolKind::Constant:
			iconName = "constant";
			toolTip = B_TRANSLATE("Constant");
			break;
		case SymbolKind::String:
			iconName = "string";
			toolTip = B_TRANSLATE("String");
			break;
		case SymbolKind::Number:
			iconName = "numeric";
			toolTip = "";
			break;
		case SymbolKind::Boolean:
			iconName = "boolean";
			toolTip = B_TRANSLATE("Boolean");
			break;
		case SymbolKind::Array:
			iconName = "array";
			toolTip = B_TRANSLATE("Array");
			break;
		case SymbolKind::Object:
			iconName = "namespace";
			toolTip = B_TRANSLATE("Object");
			break;
		case SymbolKind::Key:
			iconName = "key";
			toolTip = B_TRANSLATE("Key");
			break;
		case SymbolKind::Null: // icon not available
			iconName = "misc";
			toolTip = B_TRANSLATE("Null");
			break;
		case SymbolKind::EnumMember:
			iconName = "emum-member";
			toolTip = B_TRANSLATE("Enum member");
			break;
		case SymbolKind::Struct:
			iconName = "structure";
			toolTip = B_TRANSLATE("Structure");
			break;
		case SymbolKind::Event:
			iconName = "event";
			toolTip = B_TRANSLATE("Event");
			break;
		case SymbolKind::Operator:
			iconName = "operator";
			toolTip = B_TRANSLATE("Operator");
			break;
		case SymbolKind::TypeParameter:
			iconName = "parameter";
			toolTip = B_TRANSLATE("Type parameter");
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
				if (item != nullptr) {
					if (item->HasToolTip())
						SetToolTip(item->GetToolTipText());
					else
						SetToolTip("");
				}
			}
		}
	}
protected:
	virtual void ExpandOrCollapse(BListItem* superItem, bool expand)
	{
		BOutlineListView::ExpandOrCollapse(superItem, expand);

		BString symbol;
		static_cast<SymbolListItem*>(superItem)->Details().FindString("name", &symbol);
		BMessage message('0099');
		message.AddString("symbol", symbol);
		message.AddBool("collapsed", !expand);
		Window()->PostMessage(&message);
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
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fToolBar)
		.AddGroup(B_VERTICAL, 0)
			.SetInsets(-2, -2, -2, -2)
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

	// Save the vertical scrolling value
	BScrollBar* vertScrollBar = fScrollView->ScrollBar(B_VERTICAL);
	float scrolledValue = vertScrollBar->Value();

	Window()->DisableUpdates();

	fListView->MakeEmpty();
	_RecursiveAddSymbols(nullptr, &msg);
	fSymbolsLastUpdateTime = system_time();

	// Collapse items
	// TODO: Maybe to the opposite: have a list of collapsed items (which are less)
	// and compare the listview ONCE with the smaller list of to-be-collapsed items
	// TODO: symbol name isn't enough: we need to compare also with symbol kind
	int32 i = 0;
	BString collapsed;
	while (msg.FindString("collapsed", i, &collapsed) == B_OK) {
		for (int32 itemIndex = 0; itemIndex < fListView->CountItems(); itemIndex++) {
			BStringItem* item = static_cast<BStringItem*>(fListView->ItemAt(itemIndex));
			if (collapsed.Compare(item->Text()) == 0) {
				fListView->Collapse(item);
				break;
			}
		}
		i++;
	}
	if (sSorted)
		fListView->SortItemsUnder(nullptr, true, &CompareItemsText);

	// same document, don't reset the vertical scrolling value
	if (*newRef == fCurrentRef) {
		if (vertScrollBar->Value() != scrolledValue)
			vertScrollBar->SetValue(scrolledValue);
	}

	// List could have been changed.
	// Try to re-select old selected item, but only if it's the same document
	if (*newRef == fCurrentRef && !selectedItemText.IsEmpty()) {
		for (int32 itemIndex = 0; itemIndex < fListView->CountItems(); itemIndex++) {
			BStringItem* item = dynamic_cast<BStringItem*>(fListView->ItemAt(itemIndex));
			if (item != nullptr && selectedItemText == item->Text()) {
				fListView->Select(itemIndex, false);
				break;
			}
		}
	}

	Window()->EnableUpdates();

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
