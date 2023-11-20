/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ProblemsPanel_H
#define ProblemsPanel_H

#include <ColumnListView.h>
#include <SupportDefs.h>
#include <TabView.h>

class BPopUpMenu;
class BMenuItem;

class ProblemsPanel : public BColumnListView {
public:
		ProblemsPanel(BTabView*);
		virtual ~ProblemsPanel();

		void UpdateProblems(BMessage* msg);

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
