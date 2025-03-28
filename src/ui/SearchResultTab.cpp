/*
 * Copyright 2024, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SearchResultTab.h"

#include <Catalog.h>
#include <Button.h>
#include <LayoutBuilder.h>

#include "ActionManager.h"
#include "ConfigManager.h"
#include "GenioWindow.h"
#include "GenioWindowMessages.h"
#include "ProjectBrowser.h"
#include "SearchResultPanel.h"
#include "TextUtils.h"
#include "ToolBar.h"


extern ConfigManager gCFG;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SearchResultTab"

static constexpr auto kFindReplaceMaxBytes = 50;
static constexpr auto kFindReplaceMinBytes = 32;

static constexpr uint32 kSelectProject ='PRJX';


SearchResultTab::SearchResultTab(PanelTabManager* panelTabManager, tab_id id)
	:
	BGroupView(B_VERTICAL, 0.0f),
	fSearchResultPanel(nullptr),
	fSelectedProject(nullptr)
{
	fProjectMenu = new OptionList<ProjectFolder *>("ProjectMenu",
		B_TRANSLATE("Project:"),
		B_TRANSLATE("Choose project" B_UTF8_ELLIPSIS));

	fFindGroup = new ToolBar(this);
	ActionManager::AddItem(MSG_FIND_IN_FILES, fFindGroup);
	fFindGroup->FindButton(MSG_FIND_IN_FILES)->SetLabel(B_TRANSLATE("Find in project"));

	fFindTextControl = new BTextControl("FindTextControl", "" , "", nullptr);
	fFindTextControl->TextView()->SetMaxBytes(kFindReplaceMaxBytes);
	float charWidth = fFindTextControl->StringWidth("0", 1);
	fFindTextControl->SetExplicitMinSize(
						BSize(charWidth * kFindReplaceMinBytes + 10.0f,
						B_SIZE_UNSET));
	fFindTextControl->SetExplicitMaxSize(fFindTextControl->MinSize());

	fFindCaseSensitiveCheck = new BCheckBox(B_TRANSLATE_COMMENT("Match case", "Short as possible."),
		nullptr);
	fFindWholeWordCheck = new BCheckBox(B_TRANSLATE_COMMENT("Whole word", "Short as possible."),
		nullptr);

	fSearchResultPanel = new SearchResultPanel(panelTabManager, id);
	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
		.Add(fSearchResultPanel, 3.0f)
		.AddGroup(B_VERTICAL, 0.0f)
			.SetInsets(B_USE_SMALL_SPACING)
			.Add(fProjectMenu)
			.AddGlue()
			.Add(fFindWholeWordCheck)
			.Add(fFindCaseSensitiveCheck)
			.AddGlue()
			.Add(fFindTextControl)
			.AddGlue()
			.Add(fFindGroup)
		.End()
	.End();
}


void
SearchResultTab::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kSelectProject:
		{
			fSelectedProject = const_cast<ProjectFolder*>(
				reinterpret_cast<const ProjectFolder*>(message->GetPointer("value")));
			break;
		}
		case MSG_FIND_IN_FILES:
			_StartSearch(fFindTextControl->Text(), (bool)fFindWholeWordCheck->Value(),
				(bool)fFindCaseSensitiveCheck->Value(), fSelectedProject);
			break;
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code) != B_OK)
				break;
			switch (code) {
				case MSG_NOTIFY_PROJECT_LIST_CHANGED:
				{
					_UpdateProjectList(gMainWindow->GetProjectBrowser()->GetProjectList());
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
SearchResultTab::AttachedToWindow()
{
	BGroupView::AttachedToWindow();

	fProjectMenu->SetTarget(this);
	fProjectMenu->SetSender("SearchResultTab");
	_UpdateProjectList(gMainWindow->GetProjectBrowser()->GetProjectList());
	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		Window()->UnlockLooper();
	}

	fFindGroup->SetTarget(this);
	fFindTextControl->SetMessage(new BMessage(MSG_FIND_IN_FILES));
	fFindTextControl->SetTarget(this);
}


void
SearchResultTab::_UpdateProjectList(const ProjectFolderList* list)
{
	if (list == nullptr) {
		fProjectMenu->MakeEmpty();
		return;
	}

	//Is the current selected project still in the new list?
	bool found = _IsProjectInList(list, fSelectedProject);

	fProjectMenu->MakeEmpty();
	if (!found)
		fSelectedProject = gMainWindow->GetActiveProject();

	ProjectFolder* activeProject = gMainWindow->GetActiveProject();
	ProjectFolder* selectedProject = fSelectedProject;

	fProjectMenu->AddList(list,
		kSelectProject,
		[&active = activeProject](auto item)
		{
			BString projectName = item ? item->Name() : "";
			BString projectPath = item ? item->Path() : "";
			if (active != nullptr && active->Path() == projectPath)
				projectName.Append("*");
			return projectName;
		},
		true,
		[&selected = selectedProject](auto item)
		{
			if (item == nullptr || selected == nullptr)
				return false;
			return (item->Path() == selected->Path());
		}
	);
}


SearchResultTab::~SearchResultTab()
{
	delete fProjectMenu;
}


void
SearchResultTab::SetAndStartSearch(BString text, bool wholeWord, bool caseSensitive, ProjectFolder* project)
{
	if (project == nullptr)
		return;

	if (text.IsEmpty())
		return;

	BMenu* menu = fProjectMenu->Menu();
	if (menu == nullptr)
		return;

	if (project != fSelectedProject) {
		for (int32 i = 0; i < menu->CountItems(); i++) {
			BMessage* msg = menu->ItemAt(i)->Message();
			if (msg != nullptr && msg->GetPointer("value", nullptr) == project) {
				fSelectedProject = project;
				menu->ItemAt(i)->SetMarked(true);
				break;
			}
		}
	}
	if (project != fSelectedProject)
		return;

	fFindCaseSensitiveCheck->SetValue((bool)caseSensitive);
	fFindWholeWordCheck->SetValue((bool)wholeWord);
	fFindTextControl->SetText(text.String());

	_StartSearch(text, wholeWord, caseSensitive, project);
}


void
SearchResultTab::_StartSearch(BString text, bool wholeWord, bool caseSensitive, ProjectFolder* project)
{
	if (project == nullptr)
		return;

	if (text.IsEmpty())
		return;

	BString extraParameters;
	if (wholeWord)
		extraParameters += "w";

	if (!caseSensitive)
		extraParameters += "i";

	text.CharacterEscape("\\\n\"", '\\');

	BString grepCommand("grep");
	BString excludeDir(gCFG["find_exclude_directory"]);
	if (!excludeDir.IsEmpty()) {
		if (excludeDir.FindFirst(",") >= 0)
			grepCommand << " --exclude-dir={" << excludeDir << "}";
		else
			grepCommand << " --exclude-dir=" << excludeDir << "";
	}

	grepCommand += " -IFHrn";
	grepCommand += extraParameters;
	grepCommand += " -- ";
	grepCommand += EscapeQuotesWrap(text);
	grepCommand += " ";
	grepCommand += EscapeQuotesWrap(project->Path());

	LogInfo("Find in file, executing: [%s]", grepCommand.String());
	fSearchResultPanel->StartSearch(grepCommand, project->Path());
}


bool
SearchResultTab::_IsProjectInList(const ProjectFolderList* list, ProjectFolder* proj)
{
	// Is the current selected project still in the new list?
	for (ProjectFolder* element : *list) {
		if (element == proj) {
			return true;
		}
	}
	return false;
}
