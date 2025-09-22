/*
 * Copyright 2025, Stefano Ceccherini
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <vector>

#include <Bitmap.h>


class SpinningAnimation {
public:
	static void		Draw(BView* owner, BRect bounds);
	static status_t	InitAnimationIcons(const char* prefix, int32 num);
	static status_t DisposeAnimationIcons();
	static void		TickAnimation();

private:
	static int32	sBuildAnimationIndex;
	static std::vector<BBitmap*> sBuildAnimationFrames;
};

