/*
 * Copyright 2023-2024, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <View.h>


class BListItem;
class BStringView;
class BOutlineListView;
class ToolBar;
class FunctionsOutlineView : public BView {
public:
	// TODO: Maybe reuse LSP definition ?
	struct Symbol {
		BString name;
		int32 kind;
	};
			FunctionsOutlineView();

	void	AttachedToWindow() override;
	void	DetachedFromWindow() override;
	void	MessageReceived(BMessage* msg) override;

private:
	BListItem*  _RecursiveSymbolByCaretPosition(int32 position, BListItem* parent);
	void        _UpdateDocumentSymbols(const BMessage& msg, const entry_ref* ref);
	void	    _RecursiveAddSymbols(BListItem* parent, const BMessage* msg);
    status_t    _GoToSymbol(BMessage *msg);
	void        _RenameSymbol(BMessage *msg);
	void		_SelectSymbolByCaretPosition(int32 position);

	BOutlineListView* fListView;
	BScrollView* fScrollView;
	ToolBar*	fToolBar;
	Symbol		fSelectedSymbol;
	entry_ref	fCurrentRef;
};
