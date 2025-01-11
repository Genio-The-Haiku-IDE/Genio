/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SearchResultPanel_H
#define SearchResultPanel_H

#include <ColumnListView.h>
#include <SupportDefs.h>
#include "GrepThread.h"
#include "PanelTabManager.h"

// For now this is specific to manage only the FindInFiles results
// Can be extended to handle more generic 'search' results (find references)

class SearchResultPanel : public BColumnListView {
public:
		SearchResultPanel(PanelTabManager*, tab_id id);

		void StartSearch(BString command, BString projectPath);

		virtual void	MessageReceived(BMessage* msg);
		virtual void	AttachedToWindow();
		virtual void 	KeyDown(const char* bytes, int32 numBytes);

		void	SetTabLabel(BString label);

private:
		void	_UpdateTabLabel(const char* txt = nullptr);
		void	ClearSearch();
		void 	UpdateSearch(BMessage* msg);
		GrepThread*	fGrepThread;
		BString 	fProjectPath;
		PanelTabManager*	fPanelTabManager;
		int32		fCountResults;
		tab_id		fTabId;
};


#endif // ProblemsPanel_H
