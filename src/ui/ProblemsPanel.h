/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ProblemsPanel_H
#define ProblemsPanel_H

#include "Editor.h"
#include <ColumnListView.h>
#include <SupportDefs.h>
#include <TabView.h>

class BPopUpMenu;
class BMenuItem;

class ProblemsPanel : public BColumnListView {
public:
		ProblemsPanel(BTabView*);
		virtual ~ProblemsPanel();

		void UpdateProblems(Editor* editor);

		virtual void MessageReceived(BMessage* msg);
		virtual void AttachedToWindow();

		void ClearProblems();

private:
		void	_UpdateTabLabel();
		BTabView* fTabView;
		BPopUpMenu* fPopUpMenu;
		BMenuItem*  fQuickFixItem;
};


#endif // ProblemsPanel_H
