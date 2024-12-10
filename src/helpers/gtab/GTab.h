/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <TabView.h>
#include <Box.h>

class GenioTabView;

class GTabContainer : public BBox {
public:
	GTabContainer(BView* view);
	BView*	ContentView() { return fView;}
private:
	BView* fView;
};

class GTab : public BTab {
protected:
friend GenioTabView;
		GTab(BView* view);
private:
		GTabContainer*	fGTabContainer;
};


