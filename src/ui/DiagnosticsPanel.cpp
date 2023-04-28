/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "DiagnosticsPanel.h"
#include <ColumnTypes.h>
#include <Catalog.h>
#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiagnosticsPanel"

#include <string>

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

DiagnosticsPanel::DiagnosticsPanel(): BColumnListView(B_TRANSLATE("Diagnostics"),
									B_NAVIGABLE, B_FANCY_BORDER, true)
	
{
	AddColumn(new BStringColumn(B_TRANSLATE("Category"),
								200.0, 200.0, 200.0, 0), kCategoryColumn);
	AddColumn(new BStringColumn(B_TRANSLATE("Message"),
								600.0, 600.0, 800.0, 0), kMessageColumn);
	AddColumn(new BStringColumn(B_TRANSLATE("Source"),
								200.0, 200.0, 200.0, 0), kSourceColumn);
	
	SetInvocationMessage(new BMessage('clIV'));
}

void 
DiagnosticsPanel::AttachedToWindow()
{
	BColumnListView::AttachedToWindow();
	SetTarget(this);
}

void 
DiagnosticsPanel::MessageReceived(BMessage* msg)
{
	if (msg->what == 'clIV')
	{
		RangeRow*	range = dynamic_cast<RangeRow*>(CurrentSelection());
		if (range)
			range->fRange.PrintToStream();
		
	}
		
	BColumnListView::MessageReceived(msg);
}

void 	
DiagnosticsPanel::UpdateDiagnostics(BMessage* msg)
{
	Clear();
	BMessage dia;
	int32 index = 0;
	
	while (msg->FindMessage(std::to_string(index++).c_str(), &dia) == B_OK) {
   
		   RangeRow* row = new RangeRow();
		   dia.FindMessage("range", &row->fRange);
		   dia.PrintToStream();
		   row->SetField(new BStringField(dia.GetString("category","")), kCategoryColumn);
		   row->SetField(new BStringField(dia.GetString("message","")), kMessageColumn);
		   row->SetField(new BStringField(dia.GetString("source","")), kSourceColumn);
		   AddRow(row);
	}

}



/*
       BRow* fRow = new BRow();
       time_t now =  static_cast<bigtime_t>(real_time_clock());

       fRow->SetField(new BDateField(&now), kTimeColumn);
       fRow->SetField(new BStringField(message), kMessageColumn);
       fRow->SetField(new BStringField(type), kTypeColumn);
       //fNotificationsListView->AddRow(fRow, int32(0));
*/