/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <InterfaceDefs.h>
#include <ObjectList.h>
#include <map>

class GTabID;
class BView;
class PanelTabView;

typedef uint32  tab_id;

typedef std::map<const char*, PanelTabView*> TabViewList;


class PanelTabManager {
public:
		PanelTabManager();

		BView*	CreatePanelTabView(const char* tabview_name, orientation orientation);

		void	AddPanel(const char* tabview_name, BView* panel, tab_id id);

		void	SelectTab(tab_id id);
		void	ShowTab(tab_id id);

		void	SetLabelForTab(tab_id id, const char* label);

		void	ShowPanelTabView(const char* tabview_name, bool visible);
		bool	IsPanelTabViewVisible(const char* tabview_name);


private:
		PanelTabView*	GetPanelTabView(const char* name);
		TabViewList		fTVList;
};


