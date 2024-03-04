/*
 * Copyright 2023, Andrea Anzani 
 * Copyright 2024, Nexus6 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ProblemsPanel.h"
#include "EditorMessages.h"
#include "GMessage.h"
#include "LSPEditorWrapper.h"

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
	kSourceColumn,
	kPositionColumn
};

class RangeRow : public BRow {
	public:
		RangeRow(){};

		GMessage	fRange;
		Editor 		*fEditor;

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
	AddColumn(new BStringColumn( B_TRANSLATE("Line"),
								100.0, 20.0, 200.0, 0), kPositionColumn);
}

ProblemsPanel::~ProblemsPanel()
{
}

void
ProblemsPanel::AttachedToWindow()
{
	BColumnListView::AttachedToWindow();
	SetInvocationMessage(new BMessage(COLUMNVIEW_CLICK));
	SetSelectionMessage(new BMessage(COLUMNVIEW_SELECT));
	SetTarget(this);
}

void
ProblemsPanel::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case COLUMNVIEW_CLICK: {
			msg->PrintToStream();
			RangeRow* range = dynamic_cast<RangeRow*>(CurrentSelection());
			if (range) {
				GMessage refs = {
					{"what", B_REFS_RECEIVED},
					{"start:line", range->fRange.GetInt32("start:line", -1) + 1},
					{"start:character", range->fRange.GetInt32("start:character", -1)}
				};
				refs.AddRef("refs",  range->fEditor->FileRef());
				Window()->PostMessage(&refs);
			}
		}
		break;
		case COLUMNVIEW_SELECT: {
			msg->PrintToStream();
			BPoint where;
			uint32 buttons = 0;
			GetMouse(&where, &buttons);
			where.PrintToStream();
			printf("button = %ud\n", buttons);
			where.x += 2; // to prevent occasional select
			if (buttons & B_SECONDARY_MOUSE_BUTTON) {

				RangeRow* row = dynamic_cast<RangeRow*>(CurrentSelection());
				if (!row)
					return;

				LSPEditorWrapper* lsp = row->fEditor->GetLSPEditorWrapper();
				if (lsp) {
					LSPDiagnostic dia;

					Range range;
					range.start.line = row->fRange["start:line"];
					range.start.character = row->fRange["start:character"];
					range.end.line = row->fRange["end:line"];
					range.end.character = row->fRange["end:character"];

					int32 index = lsp->DiagnosticFromRange(range, dia);
					fPopUpMenu = new BPopUpMenu("_popup");
					fPopUpMenu->SetRadioMode(false);
					if ( index > -1 && dia.diagnostic.codeActions.value().size() > 0) {
						std::vector<CodeAction> actions = dia.diagnostic.codeActions.value();
						for (int i = 0; i < static_cast<int>(actions.size()); i++) {
							auto item = new BMenuItem(actions[i].title.c_str(),
								new GMessage({{"what", kApplyFix}, {"index", index}, {"action", i},
								{"quickFix", true}}));
							fPopUpMenu->AddItem(item);
						}
					} else {
						auto item = new BMenuItem(B_TRANSLATE("No fix available"), nullptr);
						item->SetEnabled(false);
						fPopUpMenu->AddItem(item);
					}
					fPopUpMenu->SetTargetForItems((BHandler*)row->fEditor);
					fPopUpMenu->Go(ConvertToScreen(where), true);
					delete fPopUpMenu;
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
ProblemsPanel::UpdateProblems(Editor* editor)
{
	Clear();

	LSPEditorWrapper* lsp = editor->GetLSPEditorWrapper();
	if (lsp) {
		std::vector<LSPDiagnostic> diagnostics;
		lsp->GetDiagnostics(diagnostics);
		for (auto& dia: diagnostics) {
			RangeRow* row = new RangeRow();

			GMessage range;
			range["start:line"] = dia.diagnostic.range.start.line;
			range["start:character"] = dia.diagnostic.range.start.character;
			range["end:line"] = dia.diagnostic.range.end.line;
			range["end:character"] = dia.diagnostic.range.end.character;
			row->fRange = range;
			row->fEditor = editor;
			row->fRange.AddRef("refs", editor->FileRef());
			row->SetField(new BStringField(dia.diagnostic.category.value().c_str()), kCategoryColumn);
			row->SetField(new BStringField(dia.diagnostic.message.c_str()), kMessageColumn);
			row->SetField(new BStringField(dia.diagnostic.source.c_str()), kSourceColumn);
			BString line;
			line.SetToFormat("%d", dia.diagnostic.range.start.line + 1);
			row->SetField(new BStringField(line), kPositionColumn);
			AddRow(row);
		}
		_UpdateTabLabel();
	}
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