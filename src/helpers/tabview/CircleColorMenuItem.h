/*
 * Copyright 2024, Andrea Anzani 
 * Based on ColorMenuItem.h
 *
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */
#pragma once
#include <MenuItem.h>

class CircleColorMenuItem: public BMenuItem {
public:
								CircleColorMenuItem(const char* label, rgb_color color,
									BMessage* message);
			rgb_color&			Color() { return fItemColor; };
	virtual void 				GetContentSize(float* width, float* height);

protected:
	virtual void				DrawContent();

private:
			rgb_color			fItemColor;
};


