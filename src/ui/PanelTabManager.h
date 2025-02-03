/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <InterfaceDefs.h>
#include <Message.h>

#include <map>
#include <string>

class BView;
class PanelTabView;


#define kTabViewLeft 	"left_panels"
#define kTabViewRight	"right_panels"
#define kTabViewBottom	"bottom_panels"

enum {
	kTabProblems 		= 'Tprb',
	kTabBuildLog 		= 'Tbld',
	kTabOutputLog 		= 'Tter',
	kTabSearchResult	= 'Tsea',
	kTabTerminal		= 'Tshe',

	kTabProjectBrowser  = 'Tprj',
	kTabSourceControl   = 'Tsrc',

	kTabOutlineView		= 'Touv'

};

typedef uint32  tab_id;
typedef std::map<std::string, PanelTabView*> TabViewList;


class PanelTabManager {
public:
		 PanelTabManager();
		//~PanelTabManager(){};

		void	LoadConfiguration(const BMessage& config);
		void	SaveConfiguration(BMessage& config);

		BView*	CreatePanelTabView(const char* tabview_name, orientation orientation);
		BView*  GetPanelTabView(const char* name);

		void	AddPanelByConfig(BView* panel, tab_id id);

		void	SelectTab(tab_id id);
		void	ShowTab(tab_id id);

		void	SetLabelForTab(tab_id id, const char* label);

		void	ShowPanelTabView(const char* tabview_name, bool visible);
		bool	IsPanelTabViewVisible(const char* tabview_name);

		static BMessage	DefaultConfig();


private:
		void			_AddPanel(const char* tabview_name,
								  BView* panel,
								  tab_id id,
								  int32 index=-1,
								  bool select = false);

		PanelTabView*	_GetPanelTabView(const char* name);
		TabViewList		fTVList;
		BMessage 		fConfig;
};


