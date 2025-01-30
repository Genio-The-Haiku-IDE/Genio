/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
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
EditorTabView::AddEditor(const char* label, Editor* editor, BMessage* info, int32 index)
{
	GTabEditor*	tab = new GTabEditor(label, this, editor);
	AddTab (tab, editor, index);

	BMessage message;
	if (info != nullptr)
		message = *info;
	message.what = kETVNewTab;
	message.AddRef("ref", editor->FileRef());
	message.AddPointer("editor", editor);

	fTarget.SendMessage(&message);
}


Editor*
EditorTabView::EditorBy(const entry_ref* ref)
{
	return _GetEditor(ref);
}



Editor*
EditorTabView::EditorBy(const node_ref* nodeRef)
{
	Editor* found = nullptr;
	ForEachEditor([&](Editor* editor){
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
EditorTabView::SetTabColor(const entry_ref* ref, const rgb_color& color)
{
	GTabEditor* tab = _GetTab(ref);
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
	GTab* tab = _GetTab(ref);
	if (tab != nullptr) {
		GTabView::SelectTab(tab);
		if (selInfo != nullptr) {
			fLastSelectedInfo = *selInfo;
		}
	}
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


Editor*
EditorTabView::_GetEditor(const entry_ref* ref)
{
	GTabEditor* tab = _GetTab(ref);
	return tab ? tab->GetEditor() : nullptr;
}


GTabEditor*
EditorTabView::_GetTab(const entry_ref* ref)
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
	message.AddRef("ref", gtab->GetEditor()->FileRef());
	fTarget.SendMessage(&message);
}


void
EditorTabView::ShowTabMenu(GTabEditor* tab, BPoint where)
{
	Editor*	editor = tab->GetEditor();
	for (int32 i = 0; i < fPopUpMenu->CountItems(); i++) {
		BMessage* msg = fPopUpMenu->ItemAt(i)->Message();
		if (msg != nullptr) {
			msg->SetInt32("tab_index", Container()->IndexOfTab(tab));
			if (editor != nullptr) {
				if (msg->HasRef("ref"))
					msg->ReplaceRef("ref", editor->FileRef());
				else
					msg->AddRef("ref", editor->FileRef());
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


BString
EditorTabView::TabLabel(int32 index)
{
	GTab* tab = Container()->TabAt(index);
	return tab ? tab->Label() : "";
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
		message.AddRef("ref", tab->GetEditor()->FileRef());
		fTarget.SendMessage(&message);
	}
}


Editor*
EditorTabView::EditorAt(int32 index)
{
	GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(index));
	return tab ? tab->GetEditor() : nullptr;
}


void
EditorTabView::SetTabLabel(int32 index, const char* label)
{
	Editor* editor = EditorAt(index);
	if (editor != nullptr) {
		SetTabLabel(editor, label);
	}
}


int32
EditorTabView::IndexBy(const node_ref* nodeRef) const
{
	for (int32 i = 0; i < Container()->CountTabs(); i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab != nullptr) {
			if (tab->GetEditor()->NodeRef() != nullptr &&
				*tab->GetEditor()->NodeRef() == *nodeRef) {
				return i;
			}
		}
	}
	return -1;
}


int32
EditorTabView::IndexBy(const entry_ref* ref) const
{
	BEntry entry(ref, true);
	for (int32 i = 0; i < Container()->CountTabs(); i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab != nullptr) {
			if (entry == BEntry(tab->GetEditor()->FileRef(), true)) {
				return i;
			}
		}
	}
	return -1;
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
