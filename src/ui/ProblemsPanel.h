/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <ColumnListView.h>

class BPopUpMenu;
class BMenuItem;
class BTabView;
class Editor;
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
