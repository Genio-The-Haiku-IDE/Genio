/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include <Messenger.h>

#include "GTabView.h"

class Editor;

class EditorTabView : public GTabView {
public:

	enum {
		kETBNewTab = 'etnt'
	};

	EditorTabView(BMessenger target);

	void	AddEditor(const char* label, Editor* editor, BMessage* info = nullptr, int32 index = -1);

	Editor*	EditorBy(const entry_ref* ref) const;

	void	SetTabColor(const entry_ref* ref, const rgb_color& color);
	void	SelectTab(const entry_ref* ref, BMessage* selInfo);

private:

	BMessenger	fTarget;
};


