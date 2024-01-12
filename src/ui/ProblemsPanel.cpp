/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ProblemsPanel.h"
#include "EditorMessages.h"

#include <ColumnTypes.h>
#include <Catalog.h>
#include <Window.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <string>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProblemsPanel"

enum  {
	COLUMNVIEW_CLICK = 'clIV',
	COLUMNVIEW_SELECT = 'clSE'
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

#define ProblemLabel B_TRANSLATE("Problems")

ProblemsPanel::ProblemsPanel(BTabView* tabView): BColumnListView(ProblemLabel,
									B_NAVIGABLE, B_FANCY_BORDER, true)
									, fTabView(tabView)

{
	AddColumn(new BStringColumn( B_TRANSLATE("Category"),
								200.0, 20.0, 800.0, 0), kCategoryColumn);
	AddColumn(new BStringColumn( B_TRANSLATE("Message"),
								600.0, 20.0, 2000.0, 0), kMessageColumn);
	AddColumn(new BStringColumn( B_TRANSLATE("Source"),
								100.0, 20.0, 200.0, 0), kSourceColumn);

	fPopUpMenu = new BPopUpMenu("_popup");
	fPopUpMenu->AddItem(fQuickFixItem = new BMenuItem("fix", nullptr));
	fPopUpMenu->SetRadioMode(false);
}

ProblemsPanel::~ProblemsPanel()
{
	delete fPopUpMenu;
}

void
ProblemsPanel::AttachedToWindow()
{
	BColumnListView::AttachedToWindow();
	SetInvocationMessage(new BMessage(COLUMNVIEW_CLICK));
	SetSelectionMessage(new BMessage(COLUMNVIEW_SELECT));
	fPopUpMenu->SetTargetForItems(Window());
	SetTarget(this);
}


void
ProblemsPanel::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case COLUMNVIEW_CLICK: {
			RangeRow* range = dynamic_cast<RangeRow*>(CurrentSelection());
			if (range) {
				BMessage refs = range->fRange;
				refs.what = B_REFS_RECEIVED;
				Window()->PostMessage(&refs);
			}
		}
		break;
		case COLUMNVIEW_SELECT:{
			BPoint where;
			uint32 buttons = 0;
			GetMouse(&where, &buttons);
			where.x += 2; // to prevent occasional select
			if (buttons & B_SECONDARY_MOUSE_BUTTON){

				RangeRow* range = dynamic_cast<RangeRow*>(CurrentSelection());
				if (!range)
					return;
				BMessage* inv =	new BMessage();
				*inv = range->fRange;
				inv->what = kApplyFix;
				fQuickFixItem->SetMessage(inv);
				fQuickFixItem->SetLabel(range->fRange.GetString("title", "Fix"));
				fQuickFixItem->SetEnabled(range->fRange.GetBool("quickFix", false));
				fPopUpMenu->SetTargetForItems(Window());
				fPopUpMenu->Go(ConvertToScreen(where), true);
			}
		}
		break;
		default:
			BColumnListView::MessageReceived(msg);
		break;
	};
}

void
ProblemsPanel::UpdateProblems(BMessage* msg)
{
	Clear();
	BMessage dia;
	int32 index = 0;
	entry_ref ref;
	if (msg->FindRef("ref", &ref) != B_OK)
		return;

	while (msg->FindMessage("diagnostic", index++, &dia) == B_OK) {
		RangeRow* row = new RangeRow();
		row->fRange = dia;
		row->fRange.AddRef("refs", &ref);
		row->SetField(new BStringField(dia.GetString("category","")), kCategoryColumn);
		row->SetField(new BStringField(dia.GetString("message","")), kMessageColumn);
		row->SetField(new BStringField(dia.GetString("source","")), kSourceColumn);
		AddRow(row);
	}
	_UpdateTabLabel();
}

void
ProblemsPanel::ClearProblems()
{
	Clear();
	_UpdateTabLabel();
}

void
ProblemsPanel::_UpdateTabLabel()
{
	if (!fTabView)
		return;

	BString label = ProblemLabel;
	if (CountRows() > 0) {
		label.Append(" (");
		label.Append(std::to_string(CountRows()).c_str());
		label.Append(")");
	}


	for (int32 i = 0; i < fTabView->CountTabs(); i++) {
		if (fTabView->ViewForTab(i) == this) {
			fTabView->TabAt(i)->SetLabel(label.String());
			break;
		}
	}
}
