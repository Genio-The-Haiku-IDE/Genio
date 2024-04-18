/*
 * Copyright 2023, the Genio team
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
				FunctionsOutlineView();

	virtual		void	AttachedToWindow();
	virtual		void	DetachedFromWindow();
	virtual		void	MessageReceived(BMessage* msg);
	virtual		void	Pulse();

private:
	void	_UpdateDocumentSymbols(const BMessage& msg,
									const entry_ref* ref);
	void	_RecursiveAddSymbols(BListItem* parent, const BMessage* msg);

	BOutlineListView* fListView;
	BScrollView* fScrollView;
	ToolBar*	fToolBar;
	entry_ref	fCurrentRef;
	bigtime_t	fSymbolsLastUpdateTime;
};
