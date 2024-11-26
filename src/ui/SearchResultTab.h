/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <SupportDefs.h>
#include <GroupView.h>
#include <TabView.h>

#include "SearchResultPanel.h"

class SearchResultTab : public BGroupView {
public:
			SearchResultTab(BTabView*);
	void 	StartSearch(BString command, BString projectPath);

private:
	SearchResultPanel*	fSearchResultPanel;
	BTabView*	fTabView;
};


