/*
 * Copyright 2023, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include "TabManager.h"
#include <PopUpMenu.h>
#include <MenuItem.h>

class Editor;
class EditorTabManager : public TabManager {
public:
	EditorTabManager(const BMessenger& target);
	~EditorTabManager();

	Editor*		EditorAt(int32 index) const;
	Editor*		SelectedEditor() const;
	Editor*		EditorBy(const entry_ref* ref) const;
	Editor*		EditorBy(const node_ref* nodeRef) const;

	BString		GetToolTipText(int32 index) override;

	void		ShowTabMenu(BMessenger target, BPoint where,int32 index);

private:
	BPopUpMenu* fPopUpMenu;
	BMenuItem*  fFindInBrowser;

};
