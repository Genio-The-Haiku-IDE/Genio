/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include <SupportDefs.h>
#include "GTab.h"

class Editor;

class GTabEditor : public GTabCloseButton {
public:
	GTabEditor(const char* label, const BHandler* handler, Editor* editor):
		GTabCloseButton(label, handler), fEditor(editor) {}

	BSize	MinSize() override;
	BSize	MaxSize() override;
	void	MouseDown(BPoint where) override;
	void	MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage) override;

	Editor*	GetEditor() { return fEditor; }
	void	SetColor(const rgb_color& color);
	void	SetLabel(const char* label) override;


protected:

	void		CloseButtonClicked() override;
	void	UpdateToolTip();


private:
	Editor*	fEditor;
	rgb_color	fColor;
};


