/*
 * Copyright 2023, the Genio team
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <View.h>

class BListItem;
class BStringView;
class BOutlineListView;
class FunctionsOutlineView : public BView {
public:
	enum status {
		STATUS_EMPTY = 0,
		STATUS_LOADING,
		STATUS_LOADED
	};
			FunctionsOutlineView();

				void	UpdateDocumentSymbols(BMessage* msg);

	virtual		void	MessageReceived(BMessage* msg);
	virtual		void	Pulse();
				void	SetLoadingStatus(status loadingStatus);

private:
	void	_RecursiveAddSymbols(BListItem* parent, BMessage* msg);

	BOutlineListView* fListView;
	BStringView* fLoadingView;
	entry_ref	fCurrentRef;
	status		fLoadingStatus;
	bigtime_t	fLastUpdateTime;
};
