/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ProblemsPanel_H
#define ProblemsPanel_H

#include <ColumnListView.h>
#include <SupportDefs.h>
#include <TabView.h>

class ProblemsPanel : public BColumnListView {
public:
		ProblemsPanel(BTabView*);

		void UpdateProblems(BMessage* msg);

		virtual void MessageReceived(BMessage* msg);
		virtual void AttachedToWindow();

		void ClearProblems();

private:
		void	_UpdateTabLabel();
		BTabView* fTabView;
};


#endif // ProblemsPanel_H
