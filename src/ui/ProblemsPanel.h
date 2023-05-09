/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ProblemsPanel_H
#define ProblemsPanel_H


#include <SupportDefs.h>
#include <ColumnListView.h>


class ProblemsPanel : public BColumnListView {
public:

		ProblemsPanel();
		
		void UpdateProblems(BMessage* msg);
		
		virtual void MessageReceived(BMessage* msg);
		virtual void	AttachedToWindow();
		BString	TabLabel();

};


#endif // ProblemsPanel_H
