/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <TabView.h>
#include <Box.h>

#include "GenioTabView.h"

class GTabContainer : public BBox {
public:
	GTabContainer(BView* view);;
	BView*	ContentView() { return fView;}
private:
	BView* fView;
};

class GTab : public BTab {
protected:
friend GenioTabView;
		GTab(BView* view, tab_id id);
		tab_id	Id() { return fTabId;}
private:
		GTabContainer*	fGTabContainer;
		tab_id			fTabId;
};


