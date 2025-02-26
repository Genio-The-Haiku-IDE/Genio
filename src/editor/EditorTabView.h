/*
 * Copyright 2025, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <functional>

#include <Messenger.h>
#include <PopUpMenu.h>

#include "EditorId.h"
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

	void	AddEditor(const char* label, Editor* editor, BMessage* info = nullptr);

	Editor* SelectedEditor() const;

	void	SetTabColor(Editor*, const rgb_color& color);
	void	SetTabLabel(Editor*, const char* label);
	BString	TabLabel(Editor* editor);

	void	SelectTab(const entry_ref* ref, BMessage* selInfo = nullptr);

	void	RemoveEditor(Editor* editor);

	void	SelectNext();
	void	SelectPrev();

//deprecated :

	deprecated_ int32	SelectedTabIndex() const;

	deprecated_ int32	CountTabs() const;

	deprecated_ void	SelectTab(int32 index, BMessage* selInfo = nullptr);
	deprecated_ Editor* EditorAt(int32 index);

				Editor* EditorBy(const node_ref* nodeRef);
				Editor*	EditorBy(const entry_ref* ref);
				Editor*	EditorById(editor_id id);

	void	ForEachEditor(const std::function<bool(Editor*)>& op);
	void 	ReverseForEachEditor(const std::function<bool(Editor*)>& op);

	void	AttachedToWindow() override;
	void	MessageReceived(BMessage*) override;

protected:
friend GTabEditor;

	GTab*	CreateTabView(GTab* clone) override;

	void	OnTabSelected(GTab* tab) override;

	void	ShowTabMenu(GTabEditor* tab, BPoint where);
	BString	GetToolTipText(GTabEditor* tab);

private:
			GTab*	CreateTabView(const char* label) override;
			BMenuItem* CreateMenuItem(GTab* tab) override;

			Editor*		_GetEditor_(const entry_ref* ref);
			deprecated_ GTabEditor*	_GetTab_(const entry_ref* ref);
			GTabEditor* _GetTab(Editor* editor);
			GTabEditor* _GetTab(editor_id id);

			BMessenger	fTarget;
			BPopUpMenu* fPopUpMenu;
			BMessage 	fLastSelectedInfo;
};
