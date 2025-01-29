/*
 * Copyright 2025, Andrea Anzani 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once


#include "GTab.h"

class Editor;

class GTabEditor : public GTabCloseButton {
public:
	GTabEditor(const char* label, const BHandler* handler, Editor* editor):
		GTabCloseButton(label, handler),
		fEditor(editor),
		fColor(ui_color(B_PANEL_BACKGROUND_COLOR)){}

	BSize	MinSize() override;
	BSize	MaxSize() override;
	void	MouseDown(BPoint where) override;
	void	MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage) override;
	void	DrawLabel(BView* owner, BRect frame, const BRect& updateRect, bool isFront) override;

	Editor*	GetEditor() { return fEditor; }
	void	SetColor(const rgb_color& color);
	void	SetLabel(const char* label) override;

	rgb_color	Color() { return fColor; }

protected:

	void	CloseButtonClicked() override;
	void	UpdateToolTip();
	void	DrawCircle(BView* owner, BRect& frame);

private:
	Editor*	fEditor;
	rgb_color	fColor;
};


