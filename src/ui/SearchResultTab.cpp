/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
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
#include "ProjectMenuField.h"
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
	fSearchResultPanel(nullptr)
{
	fProjectMenu = new Genio::UI::ProjectMenuField("ProjectMenu", kSelectProject);

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

	SetName(fSearchResultPanel->Name());
}


void
SearchResultTab::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kSelectProject:
		{
			// TODO: Remove ?
			break;
		}
		case MSG_FIND_IN_FILES:
		{
			BMenuItem* item = fProjectMenu->Menu()->FindMarked();
			if (item == nullptr)
				break;
			BString projectPath;
			if (item->Message()->FindString("value", &projectPath) != B_OK)
				break;

			_StartSearch(fFindTextControl->Text(), (bool)fFindWholeWordCheck->Value(),
				(bool)fFindCaseSensitiveCheck->Value(), projectPath);
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

	fFindGroup->SetTarget(this);
	fFindTextControl->SetMessage(new BMessage(MSG_FIND_IN_FILES));
	fFindTextControl->SetTarget(this);
}


SearchResultTab::~SearchResultTab()
{
	delete fProjectMenu;
}


void
SearchResultTab::SetAndStartSearch(BString text, bool wholeWord,
	bool caseSensitive, const BString& projectPath)
{
	if (text.IsEmpty())
		return;

	fFindCaseSensitiveCheck->SetValue((bool)caseSensitive);
	fFindWholeWordCheck->SetValue((bool)wholeWord);
	fFindTextControl->SetText(text.String());

	_StartSearch(text, wholeWord, caseSensitive, projectPath);
}


void
SearchResultTab::_StartSearch(BString text, bool wholeWord,
	bool caseSensitive, const BString& projectPath)
{
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
	grepCommand += EscapeQuotesWrap(projectPath);

	LogInfo("Find in file, executing: [%s]", grepCommand.String());
	fSearchResultPanel->StartSearch(grepCommand, projectPath);
}
