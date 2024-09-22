/*
 * Copyright 2024, The Genio Team
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "GenioSecondaryWindow.h"

#include <LayoutBuilder.h>
#include <GroupLayoutBuilder.h>

GenioSecondaryWindow::GenioSecondaryWindow(BRect rect, const char* name, BView* view)
	:
	BWindow(rect, name, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(view);
}