/*
 * Copyright 2024, Andrea Anzani <andrea.anzani@gmail.com>
 * Based on ColorMenuItem.h
 *
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */

#include "CircleColorMenuItem.h"

#include <Message.h>
#include <Rect.h>


CircleColorMenuItem::CircleColorMenuItem(const char* label, rgb_color color,
	BMessage *message)
	:
	BMenuItem(label, message, 0, 0),
	fItemColor(color)
{
}


void
CircleColorMenuItem::DrawContent()
{
	float width = 0;
	float height = 0;
	BMenuItem::GetContentSize(&width, &height);
	BPoint drawPoint(ContentLocation());

	drawPoint.x += (height) + 4.0f;


	Menu()->MovePenTo(drawPoint);
	BMenuItem::DrawContent();

	Menu()->PushState();

	BPoint where(ContentLocation());
	BRect circleFrame(BRect(where, where + BPoint(height,height)));

	circleFrame.right = circleFrame.left + circleFrame.Height();
	circleFrame.InsetBy(5, 5);
	Menu()->SetHighColor(Color());
	Menu()->FillEllipse(circleFrame);
	Menu()->SetHighColor(tint_color(Color(), B_DARKEN_1_TINT));
	Menu()->StrokeEllipse(circleFrame);

	Menu()->PopState();
}


void
CircleColorMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);
	*width += (*height) + 4.0f;
}
