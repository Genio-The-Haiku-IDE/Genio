/*
 * Copyright 2023, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <OutlineListView.h>

class FunctionsOutlineView : public BOutlineListView {
public:
			FunctionsOutlineView();

				void	UpdateDocumentSymbols(BMessage* msg);

	virtual		void	MessageReceived(BMessage* msg);

private:
	void	_RecursiveAddSymbols(BListItem* parent, BMessage* msg);

	entry_ref	fCurrentRef;

};

