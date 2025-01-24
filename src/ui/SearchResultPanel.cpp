/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SearchResultPanel.h"

#include <ColumnTypes.h>
#include <Catalog.h>
#include <Window.h>
#include <string>

#include "ActionManager.h"
#include "GenioWindowMessages.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SearchResultPanel"

enum  {
	SEARCHRESULT_CLICK = 'SrCl'
};

enum {
	kLocationColumn = 0,
};

class RangeRow : public BRow {
public:
	RangeRow()
	{
	};

	BMessage fRange;
};


class BBoldStringField : public BStringField {
public:
	BBoldStringField(const char* str): BStringField(str){}
};


class BFontStringColumn : public BStringColumn {
public:
	BFontStringColumn(const char* title, float width, float minWidth, float maxWidth,
			uint32 truncate, alignment align = B_ALIGN_LEFT)
		:
		BStringColumn(title, width, minWidth, maxWidth, truncate, align)
	{
	}

	void DrawField(BField* field, BRect rect, BView* parent)
	{
		if (dynamic_cast<BBoldStringField*>(field)) {
			BFont current;
			parent->GetFont(&current);
			parent->SetFont(be_bold_font);
			BStringColumn::DrawField(field, rect, parent);
			parent->SetFont(&current);
			return;
		}
		BStringColumn::DrawField(field, rect, parent);
	}
};


#define SearchResultPanelLabel B_TRANSLATE("Search results")

SearchResultPanel::SearchResultPanel(PanelTabManager* panelTabManager, tab_id id)
	:
	BColumnListView(SearchResultPanelLabel, B_NAVIGABLE, B_FANCY_BORDER, true),
	fGrepThread(nullptr),
	fPanelTabManager(panelTabManager),
	fCountResults(0),
	fTabId(id)
{
	AddColumn(new BFontStringColumn(B_TRANSLATE("Location"),
								1000.0, 20.0, 2000.0, 0), kLocationColumn);
}


void
SearchResultPanel::SetTabLabel(BString label)
{
	if (!fPanelTabManager)
		return;

	fPanelTabManager->SetLabelForTab(fTabId, label);
}


void
SearchResultPanel::StartSearch(BString command, BString projectPath)
{
	if (fGrepThread)
		return;

	fCountResults = 0;
	fProjectPath = projectPath;
	if (!fProjectPath.EndsWith("/"))
		fProjectPath.Append("/");
	ClearSearch();

	BMessage message;
	message.AddString("cmd", command);

	ActionManager::SetEnabled(MSG_FIND_IN_FILES, false);

	_UpdateTabLabel("\xe2\x8c\x9b");//U+231x
	fGrepThread = new GrepThread(&message, BMessenger(this));
	fGrepThread->Start();
}


void
SearchResultPanel::AttachedToWindow()
{
	BColumnListView::AttachedToWindow();
	SetInvocationMessage(new BMessage(SEARCHRESULT_CLICK));
	SetTarget(this);
	_UpdateTabLabel();
}


void
SearchResultPanel::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_REPORT_RESULT:
			UpdateSearch(msg);
			break;
		case SEARCHRESULT_CLICK:
		{
			RangeRow* range = dynamic_cast<RangeRow*>(CurrentSelection());
			if (range && range->fRange.what == B_REFS_RECEIVED) {
				Window()->PostMessage(&range->fRange);
			} else {
				BRow* selected = CurrentSelection();
				ExpandOrCollapse(selected, !selected->IsExpanded());
			}
			break;
		}
		case MSG_GREP_DONE:
		{
			if (fGrepThread) {
				fGrepThread->InterruptExternal();
				delete fGrepThread;
				fGrepThread = nullptr;
			}
			_UpdateTabLabel(std::to_string(fCountResults).c_str());
			ActionManager::SetEnabled(MSG_FIND_IN_FILES, true);
			break;
		}
		default:
			BColumnListView::MessageReceived(msg);
			break;
	}
}


void
SearchResultPanel::ClearSearch()
{
	Clear();
	_UpdateTabLabel();
}


void
SearchResultPanel::UpdateSearch(BMessage* msg)
{
	BString filename = msg->GetString("filename", "");
	filename.RemoveFirst(fProjectPath);
	if (filename.IsEmpty())
		return;
	RangeRow* parent = new RangeRow();
	parent->fRange.MakeEmpty();

	parent->SetField(new BBoldStringField(filename), kLocationColumn);
	AddRow(parent);
	int32 c = 0;
	BMessage line;
	BString text;
	while (msg->FindMessage("line", c++, &line) == B_OK) {
		if (line.FindString("text", &text) == B_OK) {
			RangeRow* row = new RangeRow();
			row->fRange = line;
			row->SetField(new BStringField(text), kLocationColumn);
			AddRow(row, parent);
			fCountResults++;
			if (c == 1)
				ExpandOrCollapse(parent, true);
		}
	}
}


void
SearchResultPanel::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes > 0 && bytes[0] == B_DELETE) {
		RangeRow* range = dynamic_cast<RangeRow*>(CurrentSelection());
		if (range != nullptr)
			RemoveRow(range);
	}
	BColumnListView::KeyDown(bytes, numBytes);
}


void
SearchResultPanel::_UpdateTabLabel(const char* txt)
{
	BString label = SearchResultPanelLabel;
	if (txt) {
		label.Append(" (");
		label.Append(txt);
		label.Append(")");
	}
	SetTabLabel(label);
}
