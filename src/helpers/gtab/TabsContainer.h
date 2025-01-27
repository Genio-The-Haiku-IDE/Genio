/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <GroupView.h>
#include <Invoker.h>

#include "Draggable.h"
#include "GTabView.h"

class GTab;
class TabsContainer : public BGroupView, public BInvoker {
public:

			TabsContainer(GTabView* tabView,
						  tab_affinity	affinity = 0,
						  BMessage* message = nullptr);

	void	AddTab(GTab* tab, int32 index = -1);
	GTab*	RemoveTab(GTab* tab); //just remove, not delete.
	int32 	CountTabs() const;

	GTab*	TabAt(int32 index) const;
	int32	IndexOfTab(GTab* tab) const;

	void	ShiftTabs(int32 delta, const char* src); // 0 to refresh the current state

	void	MouseDownOnTab(GTab* tab, BPoint where, const int32 buttons);

	void	FrameResized(float w, float h) override;

	void	OnDropTab(GTab* toTab, BMessage* message);

	GTab*	SelectedTab() const;
	void	SelectTab(GTab* tab, bool invoke = true);

	GTabView*	GetGTabView() const { return fGTabView; }

	tab_affinity	GetAffinity() const { return fAffinity; }

	void	DoLayout();

private:
	void	_PrintToStream();
	void	_UpdateScrolls();

	GTab*	fSelectedTab;
	GTabView*	fGTabView;
	int32		fTabShift;
	tab_affinity	fAffinity;
	bool	fFirstLayout;
};
