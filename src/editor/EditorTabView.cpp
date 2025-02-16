/*
 * Copyright 2025, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EditorTabView.h"

#include <cassert>

#include <Box.h>
#include <Catalog.h>

#include "ActionManager.h"
#include "CircleColorMenuItem.h"
#include "Editor.h"
#include "GenioWindowMessages.h"
#include "GTabEditor.h"
#include "ProjectBrowser.h"
#include "ProjectFolder.h"
#include "TabsContainer.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "EditorTabManager"

#define	kSelectByKey 'seta'



EditorTabView::EditorTabView(BMessenger target):GTabView("_editor_tabview_",
										'EDTV',
										B_HORIZONTAL,
										true,
										true), fTarget(target)
{

	fPopUpMenu = new BPopUpMenu("tabmenu", false, false, B_ITEMS_IN_COLUMN);
	ActionManager::AddItem(MSG_FILE_CLOSE, 	fPopUpMenu);
	ActionManager::AddItem(MSG_FILE_CLOSE_ALL, fPopUpMenu);
	ActionManager::AddItem(MSG_FILE_CLOSE_OTHER, fPopUpMenu);

	fPopUpMenu->AddSeparatorItem();

	ActionManager::AddItem(MSG_FIND_IN_BROWSER, fPopUpMenu);
	ActionManager::AddItem(MSG_PROJECT_MENU_SHOW_IN_TRACKER, fPopUpMenu);
	ActionManager::AddItem(MSG_PROJECT_MENU_OPEN_TERMINAL, fPopUpMenu);

	fPopUpMenu->SetTargetForItems(target);

}


EditorTabView::~EditorTabView()
{
	delete fPopUpMenu;
}


void
EditorTabView::AddEditor(const char* label, Editor* editor, BMessage* info)
{
	//by default the new editor is placed next to the selected one.

	int32 index = SelectedTabIndex() + 1;
	GTabEditor*	tab = new GTabEditor(label, this, editor);
	AddTab (tab, editor, index);

	BMessage message;
	if (info != nullptr)
		message = *info;
	message.what = kETVNewTab;
	message.AddUInt64(kEditorId, editor->Id());

	fTarget.SendMessage(&message);
}


Editor*
EditorTabView::EditorBy(const entry_ref* ref)
{
	return _GetEditor_(ref);
}



Editor*
EditorTabView::EditorBy(const node_ref* nodeRef)
{
	Editor* found = nullptr;
	ForEachEditor([&](Editor* editor) {
		if (editor->NodeRef() != nullptr && *editor->NodeRef() == *nodeRef) {
			found = editor;
			return false;
		}
		return true;
	});
	return found;
}


Editor*
EditorTabView::SelectedEditor()
{
	GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->SelectedTab());
	return tab ? tab->GetEditor() :nullptr;
}


void
EditorTabView::SetTabColor(Editor* editor, const rgb_color& color)
{
	GTabEditor* tab = _GetTab(editor);
	if (tab != nullptr) {
		tab->SetColor(color);
	}
}


void
EditorTabView::SetTabLabel(Editor* editor, const char* label)
{
	GTabEditor* tab = _GetTab(editor);
	if (tab != nullptr)
		tab->SetLabel(label);
}



BString
EditorTabView::TabLabel(Editor* editor)
{
	GTabEditor* tab = _GetTab(editor);
	return tab ? tab->Label() : "";
}


void
EditorTabView::SelectTab(const entry_ref* ref, BMessage* selInfo)
{
	GTab* tab = _GetTab_(ref);
	if (tab != nullptr) {
		GTabView::SelectTab(tab);
		if (selInfo != nullptr) {
			fLastSelectedInfo = *selInfo;
		}
	}
}


Editor*
EditorTabView::EditorById(editor_id id)
{
	Editor* found = nullptr;
	ForEachEditor([&](Editor* editor) {
		if (editor->Id() == id) {
			found = editor;
			return false;
		}
		return true;
	});
	return found;
}



void
EditorTabView::ForEachEditor(const std::function<bool(Editor*)>& op)
{
	for (int32 i = 0; i < Container()->CountTabs(); i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab != nullptr) {
			bool next = op(tab->GetEditor());
			if (!next)
				return;
		}
	}
}


void
EditorTabView::ReverseForEachEditor(const std::function<bool(Editor*)>& op)
{
	for (int32 i = Container()->CountTabs() - 1; i >= 0; i--) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab != nullptr) {
			bool next = op(tab->GetEditor());
			if (!next)
				return;
		}
	}
}


void
EditorTabView::AttachedToWindow()
{
	// Shortcuts
	for (int32 index = 1; index < 10; index++) {
		constexpr auto kAsciiPos {48};
		BMessage* selectTab = new BMessage(kSelectByKey);
		selectTab->AddInt32("index", index - 1);
		Window()->AddShortcut(index + kAsciiPos, B_COMMAND_KEY, selectTab, this);
	}
}


void
EditorTabView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kSelectByKey:
			int32 index;
			// Shortcut selection, be careful
			if (message->FindInt32("index", &index) == B_OK) {
				if (index < Container()->CountTabs())
					SelectTab(index);
			}
			break;
		default:
			GTabView::MessageReceived(message);
			break;
	}
}


Editor*
EditorTabView::_GetEditor_(const entry_ref* ref)
{
	GTabEditor* tab = _GetTab_(ref);
	return tab ? tab->GetEditor() : nullptr;
}


GTabEditor*
EditorTabView::_GetTab_(const entry_ref* ref)
{
	BEntry entry(ref, true);
	for (int32 i = 0; i < Container()->CountTabs(); i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab != nullptr && BEntry(tab->GetEditor()->FileRef(), true) == entry) {
				return tab;
		}
	}
	return nullptr;
}


GTabEditor*
EditorTabView::_GetTab(Editor* editor)
{
	for (int32 i = 0; i < Container()->CountTabs(); i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab != nullptr && editor == tab->GetEditor()) {
				return tab;
		}
	}
	return nullptr;
}


GTabEditor*
EditorTabView::_GetTab(editor_id id)
{
	for (int32 i = 0; i < Container()->CountTabs(); i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab != nullptr && tab->GetEditor()->Id() == id) {
				return tab;
		}
	}
	return nullptr;
}



GTab*
EditorTabView::CreateTabView(GTab* clone)
{
	GTabEditor* tab = dynamic_cast<GTabEditor*>(clone);
	assert(tab != nullptr);
	GTabEditor* newTab = new GTabEditor(tab->Label().String(), this, tab->GetEditor());
	newTab->SetColor(tab->Color());
	return newTab;
}


void
EditorTabView::OnTabSelected(GTab* tab)
{
	GTabEditor* gtab = dynamic_cast<GTabEditor*>(tab);
	if (gtab == nullptr)
		return;

	BMessage message;
	if (fLastSelectedInfo.IsEmpty() == false) {
		message = fLastSelectedInfo;
		fLastSelectedInfo.MakeEmpty();
	}
	message.what = kETVSelectedTab;
	message.AddUInt64(kEditorId, gtab->GetEditor()->Id());

	fTarget.SendMessage(&message);
}


void
EditorTabView::ShowTabMenu(GTabEditor* tab, BPoint where)
{
	Editor*	editor = tab->GetEditor();
	for (int32 i = 0; i < fPopUpMenu->CountItems(); i++) {
		BMessage* msg = fPopUpMenu->ItemAt(i)->Message();
		if (msg != nullptr) {
			if (editor != nullptr) {
				//TODO: remove "ref"
				if (msg->HasRef("ref"))
					msg->ReplaceRef("ref", editor->FileRef());
				else
					msg->AddRef("ref", editor->FileRef());

				if (msg->HasUInt64(kEditorId)) {
					msg->ReplaceUInt64(kEditorId, editor->Id());
				} else {
					msg->AddUInt64(kEditorId, editor->Id());
				}
			}
		}
	}

	bool isFindInBrowserEnable = ActionManager::IsEnabled(MSG_FIND_IN_BROWSER);
	ActionManager::SetEnabled(MSG_FIND_IN_BROWSER, (editor != nullptr && editor->GetProjectFolder() != nullptr));

	fPopUpMenu->Go(where, true);

	ActionManager::SetEnabled(MSG_FIND_IN_BROWSER, isFindInBrowserEnable);
}


BString
EditorTabView::GetToolTipText(GTabEditor* tab)
{
	BString label("");
	Editor* editor = tab->GetEditor();
	if (editor != nullptr) {
		label << editor->FilePath();
		ProjectFolder* project = editor->GetProjectFolder();
		if (project != nullptr) {
			if (label.StartsWith(project->Path()))
				label.Remove(0, project->Path().Length() + 1);
					// Length + 1 to also remove the path separator
			label << "\n" << B_TRANSLATE("Project") << ": " << project->Name();
			if (project->Active())
				label << " (" << B_TRANSLATE("Active") << ")";
		}
	}
	return label;
}


void
EditorTabView::RemoveEditor(Editor* editor)
{
	GTabEditor* tab = _GetTab(editor);
	if (tab != nullptr) {
		DestroyTabAndView(tab);
	}
}


void
EditorTabView::SelectNext()
{
	GTab* selected = Container()->SelectedTab();
	if (selected == nullptr)
		return;

	int32 index = Container()->IndexOfTab(selected);
	if (index < 0 || index > Container()->CountTabs() - 1)
		return;

	GTabView::SelectTab(Container()->TabAt(index+1));
}


void
EditorTabView::SelectPrev()
{
	GTab* selected = Container()->SelectedTab();
	if (selected == nullptr)
		return;

	int32 index = Container()->IndexOfTab(selected);
	if (index < 1 || index > Container()->CountTabs())
		return;

	GTabView::SelectTab(Container()->TabAt(index - 1));
}


deprecated_ int32
EditorTabView::SelectedTabIndex()
{
	GTab* selected = Container()->SelectedTab();
	if (selected == nullptr)
		return -1;

	int32 index = Container()->IndexOfTab(selected);
	if (index < 0 || index > Container()->CountTabs())
		return -1;

	return index;
}


int32
EditorTabView::CountTabs()
{
	return Container()->CountTabs();
}


void
EditorTabView::SelectTab(int32 index, BMessage* selInfo)
{
	GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(index));
	if (tab) {
		GTabView::SelectTab(tab);

		BMessage message;
		if (selInfo) {
			message = *selInfo;
		}
		message.what = kETVSelectedTab;
		message.AddUInt64(kEditorId, tab->GetEditor()->Id());
		fTarget.SendMessage(&message);
	}
}


Editor*
EditorTabView::EditorAt(int32 index)
{
	GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(index));
	return tab ? tab->GetEditor() : nullptr;
}


BMenuItem*
EditorTabView::CreateMenuItem(GTab* tab)
{
	GTabEditor* gtab = dynamic_cast<GTabEditor*>(tab);
	return gtab ? new CircleColorMenuItem(tab->Label(), gtab->Color(), nullptr) : nullptr;
}


GTab*
EditorTabView::CreateTabView(const char* label)
{
	debugger("Unsupported");
	return nullptr;
}
