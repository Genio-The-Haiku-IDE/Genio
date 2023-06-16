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

#define ProblemLabel B_TRANSLATE("Search")

SearchResultPanel::SearchResultPanel(): BColumnListView(ProblemLabel,
									B_NAVIGABLE, B_FANCY_BORDER, true)
									, fGrepThread(nullptr)

{
	AddColumn(new BStringColumn(B_TRANSLATE("Location"),
								200.0, 20.0, 2000.0, 0), kLocationColumn);
	AddColumn(new BStringColumn(B_TRANSLATE("Message"),
								80.0, 80.0, 80.0, 0), kPositionColumn);
}

void
SearchResultPanel::StartSearch(BString command)
{
	if (fGrepThread)
		return;
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
}


void
SearchResultPanel::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case MSG_REPORT_RESULT: {
			RangeRow* parent = new RangeRow();
			parent->fRange.MakeEmpty();
			parent->SetField(new BStringField(msg->GetString("filename", "")), kLocationColumn);
			AddRow(parent);
			int32 c=0;
			BMessage line;
			while (msg->FindMessage("line",c++, &line) == B_OK) {
				RangeRow* row = new RangeRow();
				row->fRange = line;
				row->SetField(new BStringField(line.GetString("text", "")), kLocationColumn);
				row->SetField(new BStringField ("?"), kPositionColumn);
				AddRow(row, parent);
			}
			msg->PrintToStream();
		}
		case SEARCHRESULT_CLICK:{
			RangeRow* range = dynamic_cast<RangeRow*>(CurrentSelection());
			if (range) {
				if (range->fRange.what == B_REFS_RECEIVED)
					Window()->PostMessage(&range->fRange);
			}
		}
		break;
		default:
			BColumnListView::MessageReceived(msg);
	};/*
	if (msg->what == SEARCHRESULT_CLICK) {
		RangeRow* range = dynamic_cast<RangeRow*>(CurrentSelection());
		if (range) {
			//range->fRange.PrintToStream();
			BMessage start;
			if (range->fRange.FindMessage("start", &start) == B_OK) {
				const int32 be_line = start.GetDouble("line", -1.0);
				const int32 lsp_char = start.GetDouble("character", -1.0);
				entry_ref ref;
				range->fRange.FindRef("ref", &ref);

				BMessage refs(B_REFS_RECEIVED);
				refs.AddInt32("be:line", be_line + 1);
				refs.AddInt32("lsp:character", lsp_char);
				refs.AddRef("refs", &ref);
				//refs.PrintToStream();
				Window()->PostMessage(&refs);
			}
		}
		return;
	}*/

}

void
SearchResultPanel::ClearSearch()
{
	Clear();
}

void
SearchResultPanel::UpdateSearch(BMessage* msg)
{
/*
	BMessage dia;
	int32 index = 0;
	entry_ref ref;
	if (msg->FindRef("ref", &ref) != B_OK)
		return;
	while (msg->FindMessage(std::to_string(index++).c_str(), &dia) == B_OK) {
		RangeRow* row = new RangeRow();
		dia.FindMessage("range", &row->fRange);
		row->fRange.AddRef("ref", &ref);
		//dia.PrintToStream();
		row->SetField(new BStringField(dia.GetString("category","")), kLocationColumn);
		row->SetField(new BStringField(dia.GetString("message","")), kPositionColumn);
		row->SetField(new BStringField(dia.GetString("source","")), kSourceColumn);
		AddRow(row);
	}*/

	printf("FIX ME\n");
}


BString
SearchResultPanel::TabLabel()
{
	BString label = ProblemLabel;
	if (CountRows() > 0) {
		label.Append(" (");
		label.Append(std::to_string(CountRows()).c_str());
		label.Append(")");
	}
	return label;
}
