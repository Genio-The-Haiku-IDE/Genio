/*
 * Copyright 2025, Stefano Ceccherini
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SpinningAnimation.h"

#include <Application.h>
#include <Resources.h>
#include <TranslationUtils.h>
#include <View.h>

#include "Log.h"


int32 SpinningAnimation::sBuildAnimationIndex = 0;
std::vector<BBitmap*> SpinningAnimation::sBuildAnimationFrames;


/* static */
void
SpinningAnimation::Draw(BView* owner, BRect bounds)
{
	try {
		const BBitmap* frame = sBuildAnimationFrames.at(sBuildAnimationIndex);
		if (frame != nullptr) {
			owner->SetDrawingMode(B_OP_ALPHA);
			bounds.left = owner->PenLocation().x + 5;
			bounds.right = bounds.left + bounds.Height();
			bounds.OffsetBy(-1, 1);
			owner->DrawBitmap(frame, frame->Bounds(), bounds);
		}
	} catch (...) {
		// nothing to do
	}
}


/* static */
status_t
SpinningAnimation::InitAnimationIcons(const char* iconNamePrefix, int32 numIcons)
{
	// TODO: icon names are "waiting-N" where N is the index
	// 1 to 6
	BResources* resources = BApplication::AppResources();
	for (int32 i = 1; i <= numIcons; i++) {
		BString name(iconNamePrefix);
		const int32 kBrightnessBreakValue = 126;
		const rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		if (base.Brightness() >= kBrightnessBreakValue)
			name.Append("light-");
		else
			name.Append("dark-");
		name << i;
		size_t size;
		const void* rawData = resources->LoadResource(B_RAW_TYPE, name.String(), &size);
		if (rawData == nullptr) {
			LogError("InitAnimationIcons: Cannot load resource");
			break;
		}
		BMemoryIO mem(rawData, size);
		BBitmap* frame = BTranslationUtils::GetBitmap(&mem);

		sBuildAnimationFrames.push_back(frame);
	}
	return B_OK;
}


/* static */
status_t
SpinningAnimation::DisposeAnimationIcons()
{
	for (std::vector<BBitmap*>::iterator i = sBuildAnimationFrames.begin();
		i != sBuildAnimationFrames.end(); i++) {
		delete *i;
	}
	sBuildAnimationFrames.clear();

	return B_OK;
}


/* static */
void
SpinningAnimation::TickAnimation()
{
	if (++SpinningAnimation::sBuildAnimationIndex >= (int32)sBuildAnimationFrames.size())
		SpinningAnimation::sBuildAnimationIndex = 0;
}
