/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ProblemsPanel.h"

#include <ColumnTypes.h>
#include <Catalog.h>
#include <Window.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <string>
#include <stdio.h>
#include "protocol_objects.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProblemsPanel"

class MouseEventsStringColumn : public BStringColumn {
public:
	MouseEventsStringColumn(const char* title, float width, float minWidth, float maxWidth,
		uint32 truncate, alignment align = B_ALIGN_LEFT)
		: BStringColumn(title, width, minWidth, maxWidth, truncate, align)
	{
		SetWantsEvents(true);
	}

	//virtual				~PopUpColumn();

	void		MouseDown(BColumnListView* parent, BRow* row,
					BField* field, BRect fieldRect, BPoint point,
					uint32 buttons) {
						BMessage msg('SBUT');
						msg.AddPoint("point", point);
						msg.AddUInt32("buttons", buttons);
						parent->Window()->PostMessage(&msg, parent);

					}
};

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

#define ProblemLabel B_TRANSLATE("Problems")

ProblemsPanel::ProblemsPanel(BTabView* tabView): BColumnListView(ProblemLabel,
									B_NAVIGABLE, B_FANCY_BORDER, true)
									, fTabView(tabView)

{
	BPopUpMenu*	menu = new BPopUpMenu("_popUp");
	menu->AddItem(new BMenuItem("Test", nullptr));

	AddColumn(new MouseEventsStringColumn( B_TRANSLATE("Category"),
								200.0, 200.0, 200.0, 0), kCategoryColumn);
	AddColumn(new MouseEventsStringColumn( B_TRANSLATE("Message"),
								600.0, 600.0, 800.0, 0), kMessageColumn);
	AddColumn(new MouseEventsStringColumn( B_TRANSLATE("Source"),
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
	switch(msg->what) {
		case B_COPY:
		case B_CUT:
		case B_PASTE:
		case B_SELECT_ALL:
			//to avoid crash! (WIP)
			break;
		case COLUMNVIEW_CLICK: {
			RangeRow* range = dynamic_cast<RangeRow*>(CurrentSelection());
			if (range) {
				Range* r = nullptr;
				ssize_t len = 0;
				range->fRange.FindData("range", B_RAW_TYPE, (const void**)&r, &len);
				if (r == nullptr || len == 0)
					return;

				const int32 be_line = r->start.line;
				const int32 lsp_char = r->start.character;
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
		break;
		case 'SBUT':
		{
			uint32 buttons = msg->GetUInt32("buttons", 0);
			if (buttons & B_SECONDARY_MOUSE_BUTTON)
			{
				BPoint p;
				if (msg->FindPoint("point", &p) == B_OK) {

					RangeRow* row = dynamic_cast<RangeRow*>(RowAt(p));
					if (!row)
						return;

					BPopUpMenu* _menu = new BPopUpMenu("_popup");
					_menu->AddItem(new BMenuItem("Quick fix", nullptr));

					row->fRange.PrintToStream();

					BMessage codeAction;
					if (row->fRange.FindMessage("codeActions", &codeAction) == B_OK){
						_menu->Go(ConvertToScreen(p));
					}
					delete _menu;

				}
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
		row->fRange.AddRef("ref", &ref);
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
