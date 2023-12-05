/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_ITEM_H
#define PROJECT_ITEM_H

#include "StyledItem.h"

#include <Bitmap.h>
#include <Font.h>
#include <Messenger.h>

#include <TextControl.h>
#include <View.h>

// TODO: make this inherit from StyledItem

class SourceItem;
class ProjectItem : public StyledItem {
public:
					ProjectItem(SourceItem *sourceFile);
					virtual ~ProjectItem();

	void 			DrawItem(BView* owner, BRect bounds, bool complete);
	void 			Update(BView* owner, const BFont* font);

	SourceItem		*GetSourceItem() const { return fSourceItem; };

	void			SetNeedsSave(bool needs);
	void			SetOpenedInEditor(bool open);

	void			InitRename(BMessage* message);
	void			AbortRename();
	void			CommitRename();

private:
	SourceItem		*fSourceItem;
	bool			fNeedsSave;
	bool			fOpenedInEditor;
	bool			fInitRename;
	BMessage*		fMessage;
	BTextControl	*fTextControl;
	BString			fPrimaryText;
	BString			fSecondaryText;

	void			_DrawText(BView* owner, const BPoint& textPoint);
	void			_DrawTextWidget(BView* owner, const BRect& textRect);
	void			_DestroyTextWidget();
};


#endif // PROJECT_ITEM_H
