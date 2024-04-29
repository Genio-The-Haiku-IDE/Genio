/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include "StyledItem.h"

class BTextControl;
class SourceItem;
class ProjectItem : public StyledItem {
public:
					ProjectItem(SourceItem *sourceFile);
					virtual ~ProjectItem();

	virtual void 	DrawItem(BView* owner, BRect bounds, bool complete);
	virtual void 	Update(BView* owner, const BFont* font);

	SourceItem		*GetSourceItem() const { return fSourceItem; };

	void			SetNeedsSave(bool needs);
	void			SetOpenedInEditor(bool open);

	void			InitRename(BView* owner, BMessage* message);
	void			AbortRename();
	void			CommitRename();

	static void		TickAnimation();

private:
	SourceItem		*fSourceItem;
	bool			fNeedsSave;
	bool			fOpenedInEditor;
	BTextControl	*fTextControl;

	static int32	sBuildAnimationIndex;

	void			_DrawBuildIndicator(BView* owner);
	void			_DestroyTextWidget();
};
