/*
 * Copyright 2025, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GTabEditor.h"


BSize
GTabEditor::MinSize()
{
	BSize size(GTabCloseButton::MinSize());
	size.width = 200.0f;
	return size;
}



BSize
GTabEditor::MaxSize()
{
	BSize size(GTabCloseButton::MaxSize());
	float extra = be_control_look->DefaultLabelSpacing();
	float labelWidth = 300.0f;
	size.width = labelWidth + extra;
	return size;
}