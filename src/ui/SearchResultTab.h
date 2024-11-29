/*
 * Copyright 2024, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <SupportDefs.h>
#include <GroupView.h>
#include <TabView.h>
#include <TextControl.h>
#include <CheckBox.h>

#include "SearchResultPanel.h"
#include "OptionList.h"
#include "ProjectFolder.h"

class ToolBar;

using Genio::UI::OptionList;

class SearchResultTab : public BGroupView {
public:
					SearchResultTab(BTabView*);
				   ~SearchResultTab();
			void 	SetAndStartSearch(BString text, bool wholeWord, bool caseSensitive, ProjectFolder* project);
			void	AttachedToWindow();
			void	MessageReceived(BMessage *message);

private:
	void 	_StartSearch(BString text, bool wholeWord, bool caseSensitive, ProjectFolder* project);
	void	_UpdateProjectList(const BObjectList<ProjectFolder>* list);
	bool	_IsProjectInList(const BObjectList<ProjectFolder>* list, ProjectFolder* proj);

	Genio::UI::OptionList<ProjectFolder*>* fProjectMenu;
	SearchResultPanel* fSearchResultPanel;
	BTabView* fTabView;
	ProjectFolder* fSelectedProject;
	ToolBar* fFindGroup;
	BTextControl* fFindTextControl;
	BCheckBox* fFindCaseSensitiveCheck;
	BCheckBox* fFindWholeWordCheck;
};


