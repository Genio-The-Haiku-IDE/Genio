/*
 * Copyright 2025, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ScintillaUtils.h"

int32
rgb_colorToSciColor(rgb_color color)
{
	return color.red | (color.green << 8) | (color.blue << 16);
}


bool
IsScintillaViewReadOnly(BScintillaView* scintilla)
{
	return scintilla->SendMessage(SCI_GETREADONLY, 0, 0);
}


bool
CanScintillaViewCut(BScintillaView* scintilla)
{
	return (scintilla->SendMessage(SCI_GETSELECTIONEMPTY, 0, 0) == 0) &&
				!IsScintillaViewReadOnly(scintilla);
}


bool
CanScintillaViewCopy(BScintillaView* scintilla)
{
	return scintilla->SendMessage(SCI_GETSELECTIONEMPTY, 0, 0) == 0;
}


bool
CanScintillaViewPaste(BScintillaView* scintilla)
{
	return scintilla->SendMessage(SCI_CANPASTE, 0, 0);
}