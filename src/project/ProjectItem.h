/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_ITEM_H
#define PROJECT_ITEM_H

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

private:
	SourceItem		*fSourceItem;
	bool			fNeedsSave;
	bool			fOpenedInEditor;
	BTextControl	*fTextControl;

	BRect			DrawIcon(BView* owner, const BRect& bounds,
							const float& iconSize) override;

	void			_DestroyTextWidget();
};


#endif // PROJECT_ITEM_H
