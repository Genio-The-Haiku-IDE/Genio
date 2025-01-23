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

	Editor*	GetEditor() { return fEditor; }
	void	SetColor(const rgb_color& color) { fColor = color; }

protected:

	void		CloseButtonClicked() override;

private:
	Editor*	fEditor;
	rgb_color	fColor;
};


