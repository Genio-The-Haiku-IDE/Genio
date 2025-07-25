/*
 * Copyright 2024-2025, The Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "FunctionsOutlineView.h"

#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <NaturalCompare.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Window.h>

#include "ConfigManager.h"
#include "Editor.h"
#include "EditorTabView.h"
#include "GenioApp.h"
#include "GenioWindow.h"
#include "GenioWindowMessages.h"
#include "GOutlineListView.h"
#include "Log.h"
#include "protocol_objects.h"
#include "StyledItem.h"
#include "ToolBar.h"
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FunctionsOutlineView"


const int32 kMsgGoToSymbol		= 'gots';
const int32 kMsgSort			= 'sort';
const int32 kMsgCollapseAll		= 'coll';
const int32 kMsgRenameSymbol	= 'rens';

static bool sSortedByName = false;
static bool sCollapsed = false;

class SymbolListItem: public StyledItem {
public:
		SymbolListItem(BMessage& details)
			:
			StyledItem(details.GetString("name")),
			fDetails(details),
			fIconName()
		{
			_SetIconAndTooltip();
		}

		BRect DrawIcon(BView* owner, const BRect& itemBounds,
					const float &iconSize) override
		{
			BString iconPrefix;
			const int32 kBrightnessBreakValue = 126;
			const rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
			if (base.Brightness() >= kBrightnessBreakValue)
				iconPrefix = "light-";
			else
				iconPrefix = "dark-";

			const BPoint iconStartingPoint(itemBounds.left + 4.0f,
				itemBounds.top + (itemBounds.Height() - iconSize) / 2.0f);

			BBitmap* icon = new BBitmap(BRect(iconSize - 1.0f), 0, B_RGBA32);
			BString iconFullName = iconPrefix.Append(fIconName);
			if (GetVectorIcon(iconFullName.String(), icon) == B_OK) {
				owner->SetDrawingMode(B_OP_ALPHA);
				owner->DrawBitmap(icon, iconStartingPoint);
			} else {
				BString error(iconFullName);
				error << ": icon not found!";
				LogError(error.String());
			}
			delete icon;

			return BRect(iconStartingPoint, BSize(iconSize, iconSize));
		}
		const BMessage& Details() const { return fDetails; }
private:
		BMessage	fDetails;
		BString		fIconName;

		void _SetIconAndTooltip();
};


void
SymbolListItem::_SetIconAndTooltip()
{
	SymbolKind symbolKind;
	Details().FindInt32("kind", reinterpret_cast<int32*>(&symbolKind));
	BString toolTip;
	switch (symbolKind) {
		case SymbolKind::File:
			fIconName = "file";
			toolTip = B_TRANSLATE("File");
			break;
		case SymbolKind::Module:
			fIconName = "namespace";
			toolTip = B_TRANSLATE("Module");
			break;
		case SymbolKind::Namespace:
			fIconName = "namespace";
			toolTip = B_TRANSLATE("Namespace");
			break;
		case SymbolKind::Package:
			fIconName = "namespace";
			toolTip = B_TRANSLATE("Package");
			break;
		case SymbolKind::Class:
			fIconName = "class";
			toolTip = B_TRANSLATE("Class");
			break;
		case SymbolKind::Method:
			fIconName = "method";
			toolTip = B_TRANSLATE("Method");
			break;
		case SymbolKind::Property:
			fIconName = "property";
			toolTip = B_TRANSLATE("Property");
			break;
		case SymbolKind::Field:
			fIconName = "field";
			toolTip = B_TRANSLATE("Field");
			break;
		case SymbolKind::Constructor:
			fIconName = "method";
			toolTip = B_TRANSLATE("Constructor");
			break;
		case SymbolKind::Enum:
			fIconName = "enum";
			toolTip = B_TRANSLATE("Enum");
			break;
		case SymbolKind::Interface:
			fIconName = "interface";
			toolTip = B_TRANSLATE("Interface");
			break;
		case SymbolKind::Function:
			fIconName = "method";
			toolTip = B_TRANSLATE("Function");
			break;
		case SymbolKind::Variable:
			fIconName = "variable";
			toolTip = B_TRANSLATE("Variable");
			break;
		case SymbolKind::Constant:
			fIconName = "constant";
			toolTip = B_TRANSLATE("Constant");
			break;
		case SymbolKind::String:
			fIconName = "string";
			toolTip = B_TRANSLATE("String");
			break;
		case SymbolKind::Number:
			fIconName = "numeric";
			// TODO: is this correct ?
			toolTip = "";
			break;
		case SymbolKind::Boolean:
			fIconName = "boolean";
			toolTip = B_TRANSLATE("Boolean");
			break;
		case SymbolKind::Array:
			fIconName = "array";
			toolTip = B_TRANSLATE("Array");
			break;
		case SymbolKind::Object:
			fIconName = "namespace";
			toolTip = B_TRANSLATE("Object");
			break;
		case SymbolKind::Key:
			fIconName = "key";
			toolTip = B_TRANSLATE("Key");
			break;
		case SymbolKind::Null: // icon unavailable
			fIconName = "misc";
			toolTip = B_TRANSLATE("Null");
			break;
		case SymbolKind::EnumMember:
			fIconName = "enum-member";
			toolTip = B_TRANSLATE("Enum member");
			break;
		case SymbolKind::Struct:
			fIconName = "structure";
			toolTip = B_TRANSLATE("Structure");
			break;
		case SymbolKind::Event:
			fIconName = "event";
			toolTip = B_TRANSLATE("Event");
			break;
		case SymbolKind::Operator:
			fIconName = "operator";
			toolTip = B_TRANSLATE("Operator");
			break;
		case SymbolKind::TypeParameter:
			fIconName = "parameter";
			toolTip = B_TRANSLATE("Type parameter");
			break;
		default:
			break;
	}

	if (!fIconName.IsEmpty())
		fIconName = fIconName.Prepend("symbol-");

	if (!toolTip.IsEmpty()) {
		const BString detail  = Details().GetString("detail", "");
		if (!detail.IsEmpty()) {
			toolTip.Append("\n").Append(detail);
		}
		SetToolTipText(toolTip);
	}
}


// sort by line
static int
CompareItemsLine(const BListItem* itemA, const BListItem* itemB)
{
	const SymbolListItem* A = static_cast<const SymbolListItem*>(itemA);
	const SymbolListItem* B = static_cast<const SymbolListItem*>(itemB);

	const int32 lineA = A->Details().GetInt32("start:line", 0);
	const int32 lineB = B->Details().GetInt32("start:line", 0);

	return lineA - lineB;
}


// sort by name
static int
CompareItemsText(const BListItem* itemA, const BListItem* itemB)
{
	return BPrivate::NaturalCompare(
		static_cast<const BStringItem*>(itemA)->Text(),
		static_cast<const BStringItem*>(itemB)->Text()
	);
}


// SymbolOutlineListView
class SymbolOutlineListView: public GOutlineListView {
public:
	SymbolOutlineListView(const char* name)
		:
		GOutlineListView(name)
	{
	}

	void MouseMoved(BPoint point, uint32 transit, const BMessage* message) override
	{
		GOutlineListView::MouseMoved(point, transit, message);
		if (transit == B_ENTERED_VIEW || transit == B_INSIDE_VIEW) {
			BString toolTipText;
			auto index = IndexOf(point);
			if (index >= 0) {
				SymbolListItem *item = dynamic_cast<SymbolListItem*>(ItemAt(index));
				if (item != nullptr && item->HasToolTip())
					toolTipText = item->GetToolTipText();
			}
			SetToolTip(toolTipText);
		}
	}

protected:
	void ExpandOrCollapse(BListItem* superItem, bool expand) override
	{
		GOutlineListView::ExpandOrCollapse(superItem, expand);
		SymbolListItem* item = dynamic_cast<SymbolListItem*>(superItem);
		if (item != nullptr) {
			BMessage message = item->Details();
			message.what = MSG_COLLAPSE_SYMBOL_NODE;
			message.AddBool("collapsed", !expand);
			gMainWindow->PostMessage(&message);
		}
	}

private:
	void ShowPopupMenu(BPoint where) override
	{
		auto index = IndexOf(where);
		if (index >= 0) {
			auto item = dynamic_cast<SymbolListItem*>(ItemAt(index));
			if (item == nullptr)
				return;

			const BMessage symbol = item->Details();
			const Position position = {
				symbol.GetInt32("start:character", -1),
				symbol.GetInt32("start:line", -1)
			};

			auto optionsMenu = new BPopUpMenu("Outline menu", false, false);
			optionsMenu->AddItem(
				new BMenuItem(B_TRANSLATE("Go to symbol"),
					new GMessage{
						{"what", kMsgGoToSymbol},
						{"index", index}}));

			BMenuItem *renameItem = new BMenuItem(B_TRANSLATE("Rename symbol" B_UTF8_ELLIPSIS),
					new GMessage{
						{"what", kMsgRenameSymbol},
						{"index", index},
						{"start:line", position.line},
						{"start:character", position.character}});
			renameItem->SetEnabled((position.line != -1 && position.character != -1));
			optionsMenu->AddItem(renameItem);
			optionsMenu->SetTargetForItems(Target());
			optionsMenu->Go(ConvertToScreen(where), true);
			delete optionsMenu;
		}
	}
};


// FunctionsOutlineView
FunctionsOutlineView::FunctionsOutlineView()
	:
	BView(B_TRANSLATE("Outline"), B_WILL_DRAW),
	fListView(nullptr),
	fScrollView(nullptr),
	fToolBar(nullptr)
{
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
		.AddGroup(B_VERTICAL, 0)
			.Add(fToolBar)
			.AddGroup(B_VERTICAL, 0)
				.SetInsets(-2, -2, -2, -2)
				.Add(fScrollView)
				.End()
		.End();
}


/* virtual */
void
FunctionsOutlineView::AttachedToWindow()
{
	BView::AttachedToWindow();
	fToolBar->SetTarget(this);
	if (gMainWindow->LockLooper()) {
		gMainWindow->StartWatching(this, MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED);
		gMainWindow->StartWatching(this, MSG_NOTIFY_EDITOR_POSITION_CHANGED);
		gMainWindow->UnlockLooper();
	}

	// TODO: We do not handle the "setting change" notice,
	// but it isn't so important, since the setting is hidden
	sSortedByName = gCFG["outline_sort_symbols"];
	fToolBar->SetActionPressed(kMsgSort, sSortedByName);
}


/* virtual */
void
FunctionsOutlineView::DetachedFromWindow()
{
	if (gMainWindow != nullptr && gMainWindow->LockLooper()) {
		gMainWindow->StopWatching(this, MSG_NOTIFY_EDITOR_POSITION_CHANGED);
		gMainWindow->StopWatching(this, MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED);
		gMainWindow->UnlockLooper();
	}
	BView::DetachedFromWindow();
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
				case MSG_NOTIFY_EDITOR_POSITION_CHANGED:
				{
					int32 line;
					if (msg->FindInt32("line", &line) == B_OK)
						_SelectSymbolByCaretPosition(line);
					break;
				}
				case MSG_NOTIFY_EDITOR_SYMBOLS_UPDATED:
				{
					entry_ref newRef;
					msg->FindRef("ref", &newRef);

					// TODO: Find a way to avoid this call: if we ever manage to
					// implement undockable tabs, this call would need to lock the main window
					// and it's not nice
					Editor* editor = gMainWindow->TabManager()->SelectedEditor();
					// Got a message from an unselected editor: ignore.
					if (editor != nullptr && *editor->FileRef() != newRef) {
						LogTrace("Outline view got a message from an unselected editor. Ignoring...");
						return;
					}

					BMessage symbols;
					if (msg->FindMessage("symbols", &symbols) != B_OK) {
						debugger("No symbols message");
						break;
					}
					LogTrace("FunctionsOutlineView: Symbols updated message received");

					_UpdateDocumentSymbols(symbols, &newRef);
					_SelectSymbolByCaretPosition(msg->GetInt32("caret_line", -1));
					break;
				}
				default:
					BView::MessageReceived(msg);
					break;
			}
			break;
		}
		case kMsgGoToSymbol:
		{
			_GoToSymbol(msg);
			break;
		}
		case kMsgRenameSymbol:
		{
			// TODO: GoToSymbol is not synchronous,
			// so this won't work always correctly
			if (_GoToSymbol(msg) == B_OK)
				_RenameSymbol(msg);
			break;
		}
		case kMsgSort:
		{
			sSortedByName = !sSortedByName;
			gCFG["outline_sort_symbols"] = sSortedByName;

			fToolBar->SetActionPressed(kMsgSort, sSortedByName);
			if (sSortedByName)
				fListView->FullListSortItems(&CompareItemsText);
			else
				fListView->FullListSortItems(&CompareItemsLine);
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
FunctionsOutlineView::_SelectSymbolByCaretPosition(int32 position)
{
	BListItem* sym = _RecursiveSymbolByCaretPosition(position, nullptr);
	if (sym != nullptr && !sym->IsSelected()) {
		fListView->Select(fListView->IndexOf(sym));
		fListView->ScrollToSelection();
	}
}


BListItem*
FunctionsOutlineView::_RecursiveSymbolByCaretPosition(int32 position, BListItem* parent)
{
	for (int32 i = 0; i < fListView->CountItemsUnder(parent, true); i++) {
		SymbolListItem* sym = dynamic_cast<SymbolListItem*>(fListView->ItemUnderAt(parent, true, i));
		if (sym == nullptr)
			return nullptr;
		if (position >= sym->Details().GetInt32("range:start:line", -1) &&
			position <= sym->Details().GetInt32("range:end:line", -1) ) {

			if (fListView->CountItemsUnder(sym, true) > 0 && sym->IsExpanded()) {
				BListItem* subSym = _RecursiveSymbolByCaretPosition(position, sym);
				if (subSym != nullptr)
					return subSym;
			}

			return sym;
		}
	}
	return nullptr;
}


void
FunctionsOutlineView::_UpdateDocumentSymbols(const BMessage& msg, const entry_ref* newRef)
{
	LogTrace("FunctionsOutlineView::_UpdateDocumentSymbol()");

	const int32 status = msg.GetInt32("status", Editor::STATUS_UNKNOWN);
	switch (status) {
		case Editor::STATUS_UNKNOWN:
			fListView->MakeEmpty();
			return;
		case Editor::STATUS_NO_CAPABILITY:
			fListView->MakeEmpty();
			fListView->AddItem(new BStringItem(B_TRANSLATE("No outline available")));
			return;
		case Editor::STATUS_REQUESTED:
			fListView->MakeEmpty();
			fListView->AddItem(new BStringItem(B_TRANSLATE("Creating outline")));
			return;
		default:
			break;
	}

	// Save selected item
	SymbolListItem* selected = dynamic_cast<SymbolListItem*>(fListView->ItemAt(fListView->CurrentSelection()));
	if (selected != nullptr && *newRef == fCurrentRef) {
		fSelectedSymbol.name = selected->Details().GetString("name");
		fSelectedSymbol.kind = selected->Details().GetInt32("kind", -1);
	} else {
		fSelectedSymbol.name = "";
	}
	// Save the vertical scrolling value
	BScrollBar* vertScrollBar = fScrollView->ScrollBar(B_VERTICAL);
	const float scrolledValue = vertScrollBar->Value();

	Window()->DisableUpdates();

	fListView->MakeEmpty();
	_RecursiveAddSymbols(nullptr, &msg);

	// Collapse items
	// TODO: Maybe to the opposite: have a list of collapsed items (which are less)
	// and compare the listview ONCE with the smaller list of to-be-collapsed items
	// TODO: symbol name isn't enough: we need to compare also with symbol kind
	int32 i = 0;
	BString collapsedName;
	int32 collapsedKind = -1;
	while (msg.FindString("collapsed_name", i, &collapsedName) == B_OK) {
		msg.FindInt32("collapsed_kind", i, &collapsedKind);
		for (int32 itemIndex = 0; itemIndex < fListView->CountItems(); itemIndex++) {
			SymbolListItem* item = static_cast<SymbolListItem*>(fListView->ItemAt(itemIndex));
			if (collapsedName.Compare(item->Details().GetString("name"))== 0
				&& collapsedKind == item->Details().GetInt32("kind", -1)) {
				fListView->Collapse(item);
				break;
			}
		}
		i++;
	}
	fToolBar->SetActionPressed(kMsgSort, sSortedByName);
	if (sSortedByName)
		fListView->FullListSortItems(&CompareItemsText);
	else
		fListView->FullListSortItems(&CompareItemsLine);

	// same document, don't reset the vertical scrolling value
	if (*newRef == fCurrentRef) {
		if (vertScrollBar->Value() != scrolledValue)
			vertScrollBar->SetValue(scrolledValue);
	}

	Window()->EnableUpdates();

	fCurrentRef = *newRef;

	fListView->SetInvocationMessage(new BMessage(kMsgGoToSymbol));
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

		if (symbol.GetString("name") == fSelectedSymbol.name &&
			symbol.GetInt32("kind", -1) == fSelectedSymbol.kind)
			fListView->Select(fListView->IndexOf(item), false);

		BMessage child;
		if (symbol.FindMessage("children", &child) == B_OK) {
			_RecursiveAddSymbols(item, &child);
		}
	}
}


status_t
FunctionsOutlineView::_GoToSymbol(BMessage *msg)
{
	status_t status = B_ERROR;
	const int32 index = msg->GetInt32("index", -1);
	if (index > -1) {
		SymbolListItem* sym = dynamic_cast<SymbolListItem*>(fListView->ItemAt(index));
		if (sym != nullptr) {
			BMessage go = sym->Details();
			go.what = B_REFS_RECEIVED;
			go.AddRef("refs", &fCurrentRef);
			gMainWindow->PostMessage(&go);
			status = B_OK;
		}
	}
	return status;
}


void
FunctionsOutlineView::_RenameSymbol(BMessage *msg)
{
	BMessage go(MSG_RENAME);
	gMainWindow->PostMessage(&go);
}
