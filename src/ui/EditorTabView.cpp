/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "EditorTabView.h"
#include <Box.h>
#include <cassert>
#include "Editor.h"
#include "TabsContainer.h"
#include "GTab.h"

class GTabEditor : public GTabCloseButton {
public:
	GTabEditor(const char* label, const BHandler* handler, Editor* editor):
		GTabCloseButton(label, handler), fEditor(editor) {}

	Editor*	GetEditor() { return fEditor; }
	void	SetColor(const rgb_color& color) { fColor = color; }
private:
	Editor*	fEditor;
	rgb_color	fColor;
};

EditorTabView::EditorTabView(BMessenger target):GTabView("_editor_tabview_",
										'EDTV',
										B_HORIZONTAL,
										true,
										true), fTarget(target)
{

}


void
EditorTabView::AddEditor(const char* label, Editor* editor, BMessage* info, int32 index)
{
	GTabEditor*	tab = new GTabEditor(label, this, editor);
	AddTab (tab, editor, index);

	BMessage message;
	if (info)
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
	if (tab) {
		tab->SetColor(color);
	}
}


void
EditorTabView::SetTabLabel(Editor* editor, const char* label)
{
	GTabEditor* tab = _GetTab(editor);
	if (tab)
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
	if (tab) {
		GTabView::SelectTab(tab, false);

		BMessage message;
		if (selInfo) {
			message = *selInfo;
		}
		message.what = kETVSelectedTab;
		message.AddRef("ref", ref);
		fTarget.SendMessage(&message);
	}
}


void
EditorTabView::ForEachEditor(const std::function<bool(Editor*)>& op) {
	for (int32 i=0;i<Container()->CountTabs();i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab) {
			bool next = op(tab->GetEditor());
			if (next == false)
				return;
		}
	}
}


void
EditorTabView::ReverseForEachEditor(const std::function<bool(Editor*)>& op) {
	for (int32 i = Container()->CountTabs() - 1; i >=0; i--) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab) {
			bool next = op(tab->GetEditor());
			if (next == false)
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
	for (int32 i=0;i<Container()->CountTabs();i++) {
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
	for (int32 i=0;i<Container()->CountTabs();i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab != nullptr && editor == tab->GetEditor()) {
				return tab;
		}
	}
	return nullptr;
}


void
EditorTabView::OnTabRemoved(GTab* _tab)
{
}


void
EditorTabView::OnTabAdded(GTab* _tab, BView* panel)
{
}


GTab*
EditorTabView::CreateTabView(GTab* clone)
{
	GTabEditor* tab = dynamic_cast<GTabEditor*>(clone);
	assert(tab != nullptr);
	return new GTabEditor(tab->Label().String(), this, tab->GetEditor());
}



void
EditorTabView::OnTabSelected(GTab* tab)
{
	GTabEditor* gtab = dynamic_cast<GTabEditor*>(tab);
	if (gtab == nullptr)
		return;
	BMessage message(kETVSelectedTab);
	message.AddRef("ref", gtab->GetEditor()->FileRef());
	fTarget.SendMessage(&message);
}


void
EditorTabView::RemoveEditor(Editor* editor)
{
	GTabEditor* tab = _GetTab(editor);
	if (tab) {
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

	Container()->SelectTab(Container()->TabAt(index+1));

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

	Container()->SelectTab(Container()->TabAt(index - 1));
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
		GTabView::SelectTab(tab, false);

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
EditorTabView::RemoveTab(int32 index)
{
	GTab* tab = Container()->TabAt(index);
	if (tab)
		DestroyTabAndView(tab);
}


void
EditorTabView::SetTabLabel(int32 index, const char* label)
{
	Editor* editor = EditorAt(index);
	if (editor) {
		SetTabLabel(editor, label);
	}
}


int32
EditorTabView::IndexBy(const node_ref* nodeRef) const
{
	for (int32 i=0;i<Container()->CountTabs();i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab) {
			if ( tab->GetEditor()->NodeRef() != nullptr &&
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
	for (int32 i=0;i<Container()->CountTabs();i++) {
		GTabEditor* tab = dynamic_cast<GTabEditor*>(Container()->TabAt(i));
		if (tab) {
			if ( entry == BEntry(tab->GetEditor()->FileRef(), true)) {
				return i;
			}
		}
	}
	return -1;
}