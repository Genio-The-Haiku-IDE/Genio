/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <functional>
#include "GTabView.h"

class Editor;
class GTabEditor;

#define deprecated_

class EditorTabView : public GTabView {
public:

	enum {
		kETVNewTab = 'etnt',
		kETVSelectedTab = 'etst',
		kETVCloseTab = 'etct'
	};

			 EditorTabView(BMessenger target);
			~EditorTabView();

	void	AddEditor(const char* label, Editor* editor, BMessage* info = nullptr, int32 index = -1);

	Editor*	EditorBy(const entry_ref* ref);
	Editor* SelectedEditor();

	void	SetTabColor(const entry_ref* ref, const rgb_color& color);
	void	SetTabLabel(Editor*, const char* label);
	BString	TabLabel(Editor* editor);

	void	SelectTab(const entry_ref* ref, BMessage* selInfo = nullptr);

	void	RemoveEditor(Editor* editor);

	void	SelectNext();
	void	SelectPrev();

//deprecated :

	deprecated_ int32	SelectedTabIndex();

	deprecated_ int32	CountTabs();
	deprecated_ BString TabLabel(int32 index);
	deprecated_ void	SelectTab(int32 index, BMessage* selInfo = nullptr);
	deprecated_ Editor* EditorAt(int32 index);
	deprecated_ void	SetTabLabel(int32 index, const char* label);
	deprecated_ Editor* EditorBy(const node_ref* nodeRef);
	deprecated_ int32	IndexBy(const node_ref* nodeRef) const;
	deprecated_ int32	IndexBy(const entry_ref* ref) const;

/////////

	void ForEachEditor(const std::function<bool(Editor*)>& op);
	void ReverseForEachEditor(const std::function<bool(Editor*)>& op);

protected:
friend GTabEditor;

	GTab* CreateTabView(GTab* clone) override;

	void	OnTabSelected(GTab* tab) override;

	void	ShowTabMenu(GTabEditor* tab, BPoint where);
	BString	GetToolTipText(GTabEditor* tab);

private:
			GTab*	CreateTabView(const char* label) override;
			BMenuItem* CreateMenuItem(GTab* tab) override;

			Editor*		_GetEditor(const entry_ref* ref);
			GTabEditor*	_GetTab(const entry_ref* ref);
			GTabEditor* _GetTab(Editor* editor);

			BMessenger	fTarget;
			BPopUpMenu* fPopUpMenu;
			BMessage 	fLastSelectedInfo;
};


