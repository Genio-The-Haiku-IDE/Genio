/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include "TabManager.h"
#include <PopUpMenu.h>

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

	void		ShowTabMenu(BMessenger target, BPoint where);
	void 		HandleTabMenuAction(BMessage* message);

private:
	BPopUpMenu* fPopUpMenu;
};
