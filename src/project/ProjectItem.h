/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include "StyledItem.h"

#include <vector>

class BBitmap;
class BTextControl;
class SourceItem;
class ProjectItem : public StyledItem {
public:
					ProjectItem(SourceItem *sourceFile);
					virtual ~ProjectItem();

	void 			DrawItem(BView* owner, BRect bounds, bool complete) override;
	void 			Update(BView* owner, const BFont* font) override;

	SourceItem		*GetSourceItem() const { return fSourceItem; };

	void			SetNeedsSave(bool needs);
	void			SetOpenedInEditor(bool open);

	void			InitRename(BView* owner, BMessage* message);
	void			AbortRename();
	void			CommitRename();

protected:
	BRect			DrawIcon(BView* owner, const BRect& bounds,
							const float& iconSize) override;

private:
	SourceItem		*fSourceItem;
	bool			fNeedsSave;
	bool			fOpenedInEditor;
	bool			fRenaming;

	static BTextControl	*sTextControl;

	void			_DestroyTextWidget();
};


class ProjectTitleItem : public ProjectItem {
public:
	ProjectTitleItem(SourceItem *sourceFile);
	virtual ~ProjectTitleItem();

	void 			DrawItem(BView* owner, BRect bounds, bool complete) override;

	static status_t	InitAnimationIcons();
	static status_t DisposeAnimationIcons();
	static void		TickAnimation();

private:
	static int32	sBuildAnimationIndex;
	static std::vector<BBitmap*> sBuildAnimationFrames;

	BRect			DrawIcon(BView* owner, const BRect& bounds,
							const float& iconSize) override;

	void			_DrawBuildIndicator(BView* owner, BRect bounds);
};