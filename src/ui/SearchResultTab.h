/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <CheckBox.h>
#include <GroupView.h>
#include <TextControl.h>

#include "OptionList.h"
#include "ProjectFolder.h"
#include "SearchResultPanel.h"
#include "PanelTabManager.h"

class ToolBar;

using Genio::UI::OptionList;

class SearchResultTab : public BGroupView {
public:
					SearchResultTab(PanelTabManager* panelTabManager, tab_id id);
				   ~SearchResultTab();
			void 	SetAndStartSearch(BString text, bool wholeWord, bool caseSensitive, ProjectFolder* project);
			void	AttachedToWindow() override;
			void	MessageReceived(BMessage *message) override;

private:
	void 	_StartSearch(BString text, bool wholeWord, bool caseSensitive, ProjectFolder* project);
	void	_UpdateProjectList(const ProjectFolderList* list);
	bool	_IsProjectInList(const ProjectFolderList* list, ProjectFolder* proj);

	Genio::UI::OptionList<ProjectFolder*>* fProjectMenu;
	SearchResultPanel* fSearchResultPanel;
	ProjectFolder* fSelectedProject;
	ToolBar* fFindGroup;
	BTextControl* fFindTextControl;
	BCheckBox* fFindCaseSensitiveCheck;
	BCheckBox* fFindWholeWordCheck;
};
