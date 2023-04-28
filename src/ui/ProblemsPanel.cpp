/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ProblemsPanel.h"
#include <ColumnTypes.h>
#include <Catalog.h>
#include <Window.h>
#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProblemsPanel"

#include <string>

enum  {
	COLUMNVIEW_CLICK = 'clIV'
};

enum {
	kCategoryColumn = 0,
	kMessageColumn,
	kSourceColumn
};

class RangeRow : public BRow {
	
	public:
		RangeRow(){};
			
		BMessage	fRange;

};

ProblemsPanel::ProblemsPanel(): BColumnListView(B_TRANSLATE("Problems"),
									B_NAVIGABLE, B_FANCY_BORDER, true)
	
{
	AddColumn(new BStringColumn(B_TRANSLATE("Category"),
								200.0, 200.0, 200.0, 0), kCategoryColumn);
	AddColumn(new BStringColumn(B_TRANSLATE("Message"),
								600.0, 600.0, 800.0, 0), kMessageColumn);
	AddColumn(new BStringColumn(B_TRANSLATE("Source"),
								200.0, 200.0, 200.0, 0), kSourceColumn);
	
}

void 
ProblemsPanel::AttachedToWindow()
{
	BColumnListView::AttachedToWindow();
	SetInvocationMessage(new BMessage(COLUMNVIEW_CLICK));
	SetTarget(this);
}

void 
ProblemsPanel::MessageReceived(BMessage* msg)
{
	if (msg->what == COLUMNVIEW_CLICK)
	{
		RangeRow*	range = dynamic_cast<RangeRow*>(CurrentSelection());
		if (range) {
			//range->fRange.PrintToStream();
			BMessage start;
			if (range->fRange.FindMessage("start", &start) == B_OK) {
				const int32 be_line   = start.GetDouble("line", -1.0);
				const int32 lsp_char  = start.GetDouble("character", -1.0);
				entry_ref	ref;
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
	}
		
	BColumnListView::MessageReceived(msg);
}

void 	
ProblemsPanel::UpdateProblems(BMessage* msg)
{
	Clear();
	BMessage dia;
	int32 index = 0;
	entry_ref ref;
	if(msg->FindRef("ref", &ref) != B_OK)
		return;
	while (msg->FindMessage(std::to_string(index++).c_str(), &dia) == B_OK) {
   
		   RangeRow* row = new RangeRow();
		   dia.FindMessage("range", &row->fRange);
		   row->fRange.AddRef("ref", &ref);
		   //dia.PrintToStream();
		   row->SetField(new BStringField(dia.GetString("category","")), kCategoryColumn);
		   row->SetField(new BStringField(dia.GetString("message","")), kMessageColumn);
		   row->SetField(new BStringField(dia.GetString("source","")), kSourceColumn);
		   AddRow(row);
	}

}
