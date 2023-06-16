/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SearchResultPanel.h"

#include <ColumnTypes.h>
#include <Catalog.h>
#include <Window.h>
#include <string>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SearchResultPanel"

enum  {
	SEARCHRESULT_CLICK = 'SrCl'
};

enum {
	kLocationColumn = 0,
	kPositionColumn,
};

class RangeRow : public BRow {
	public:
		RangeRow(){};

		BMessage	fRange;

};

#define SearchResultPanelLabel B_TRANSLATE("Search Results")

SearchResultPanel::SearchResultPanel(): BColumnListView(SearchResultPanelLabel,
									B_NAVIGABLE, B_FANCY_BORDER, true)
									, fGrepThread(nullptr)

{
	AddColumn(new BStringColumn(B_TRANSLATE("Location"),
								1000.0, 20.0, 2000.0, 0), kLocationColumn);
	AddColumn(new BStringColumn(B_TRANSLATE("Message"),
								80.0, 80.0, 80.0, 0), kPositionColumn);

	//SetFont(be_fixed_font);
}

void
SearchResultPanel::StartSearch(BString command, BString projectPath)
{
	if (fGrepThread)
		return;
	fProjectPath = projectPath;
	if (!fProjectPath.EndsWith("/"))
		fProjectPath.Append("/");
	ClearSearch();
	BMessage message;
	message.AddString("cmd", command);
	fGrepThread = new GrepThread(&message, BMessenger(this));
	fGrepThread->Start();
}


void
SearchResultPanel::AttachedToWindow()
{
	BColumnListView::AttachedToWindow();
	SetInvocationMessage(new BMessage(SEARCHRESULT_CLICK));
	SetTarget(this);
	UpdateTabLabel();
}


void
SearchResultPanel::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case MSG_REPORT_RESULT:
			UpdateSearch(msg);
		break;
		case SEARCHRESULT_CLICK:{
			RangeRow* range = dynamic_cast<RangeRow*>(CurrentSelection());
			if (range && range->fRange.what == B_REFS_RECEIVED) {
				Window()->PostMessage(&range->fRange);
			}
		}
		break;
		case MSG_GREP_DONE: {
			if (fGrepThread) {
				fGrepThread->InterruptExternal();
				fGrepThread->Kill();
				delete fGrepThread;
				fGrepThread = nullptr;
			}
		}
		default:
			BColumnListView::MessageReceived(msg);
	};
}

void
SearchResultPanel::ClearSearch()
{
	Clear();
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

	parent->SetField(new BStringField(filename), kLocationColumn);
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
			if (c == 1)
				ExpandOrCollapse(parent, true);
		}
	}
	//msg->PrintToStream();
}


void
SearchResultPanel::UpdateTabLabel()
{
	BString label = SearchResultPanelLabel;
	if (CountRows() > 0) {
		label.Append(" (");
		label.Append(std::to_string(CountRows()).c_str());
		label.Append(")");
	}
}
