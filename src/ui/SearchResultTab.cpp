/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SearchResultTab.h"
#include "SearchResultPanel.h"
#include <LayoutBuilder.h>
#include <Catalog.h>
#include "GenioWindow.h"
#include "GenioWindowMessages.h"
#include "ProjectBrowser.h"
#include "TextUtils.h"
#include "ConfigManager.h"

extern ConfigManager gCFG;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SearchResultTab"

static constexpr auto kFindReplaceMaxBytes = 50;
static constexpr auto kFindReplaceMinBytes = 32;


SearchResultTab::SearchResultTab(BTabView* tabView)
	: BGroupView(B_VERTICAL, 0.0f)
	, fSearchResultPanel(nullptr)
	, fTabView(tabView)
	, fSelectedProject(nullptr)
{
	fProjectMenu = new OptionList<ProjectFolder *>("ProjectMenu",
		B_TRANSLATE("Project:"),
		B_TRANSLATE("Choose project" B_UTF8_ELLIPSIS));

	fSearchButton = new BButton(B_TRANSLATE("Find in project"), new BMessage('SERC'));

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

	fSearchResultPanel = new SearchResultPanel(fTabView);
	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
		.Add(fSearchResultPanel, 3.0f)
		.AddGroup(B_VERTICAL, 0.0f)
			.SetInsets(B_USE_SMALL_SPACING)
			.Add(fProjectMenu)
			.AddGlue()
			//.AddGroup(B_HORIZONTAL, 0.0f)
				.Add(fFindWholeWordCheck)
				.Add(fFindCaseSensitiveCheck)
			//	.End()
			.AddGlue()
			.Add(fFindTextControl)/*
			.Add(fWrapEnabled)
			.Add(fBannerEnabled)*/
			.AddGlue()
			.Add(fSearchButton)
			/*.Add(fStopButton)*/
		.End()
	.End();


}



void
SearchResultTab::MessageReceived(BMessage *message)
{
		switch (message->what) {
			case 'PRJX': {
			fSelectedProject = const_cast<ProjectFolder*>(
				reinterpret_cast<const ProjectFolder*>(message->GetPointer("value")));
			}
			break;
			case 'SERC': {
				//StartSearch();
				break;
			}
			case B_OBSERVER_NOTICE_CHANGE: {
				int32 code;
				message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
				switch (code) {
					case MSG_NOTIFY_PROJECT_LIST_CHANGED:
					{
						_UpdateProjectList(gMainWindow->GetProjectBrowser()->GetProjectList());
						break;
					}
					default:
					break;
				}
			}
			break;
			default:
				BGroupView::MessageReceived(message);
			break;
	}

}

void
SearchResultTab::AttachedToWindow()
{
	fProjectMenu->SetTarget(this);
	fProjectMenu->SetSender("SearchResultTab");
	_UpdateProjectList(gMainWindow->GetProjectBrowser()->GetProjectList());
	if (Window()->LockLooper()) {
		Window()->StartWatching(this, MSG_NOTIFY_PROJECT_LIST_CHANGED);
		Window()->UnlockLooper();
	}
	fSearchButton->SetTarget(this);
}

void
SearchResultTab::_UpdateProjectList(const BObjectList<ProjectFolder>* list)
{
	if (list == nullptr) {
		fProjectMenu->MakeEmpty();
		return;
	}

	//Is the current selected project still in the new list?
	bool found = false;
	auto count = list->CountItems();
	for (int index = 0; index < count; index++) {
		ProjectFolder* element = list->ItemAt(index);
		if (element == fSelectedProject) {
			found = true;
			break;
		}
	}

	fProjectMenu->MakeEmpty();
	if (!found)
		fSelectedProject = gMainWindow->GetActiveProject();

	ProjectFolder* activeProject = gMainWindow->GetActiveProject();
	ProjectFolder* selectedProject = fSelectedProject;


	fProjectMenu->AddList(list,
		'PRJX', //FIX ME: add constant
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
	if (!project)
		return;

	if (text.IsEmpty())
		return;

	// convert checkboxes to grep parameters..
	BString extraParameters;
	if (wholeWord)
		extraParameters += "w";

	if (caseSensitive == false)
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

