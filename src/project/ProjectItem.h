/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_ITEM_H
#define PROJECT_ITEM_H

#include <Bitmap.h>
#include <Font.h>
#include <Messenger.h>
#include <StringItem.h>
#include <TextControl.h>
#include <View.h>


class SourceItem;

class ProjectItem : public BStringItem {
public:
					ProjectItem(SourceItem *sourceFile);
					~ProjectItem();

	void 			DrawItem(BView* owner, BRect bounds, bool complete);
	void 			Update(BView* owner, const BFont* font);
	BRect 			GetTextRect() { return fTextRect; }

	SourceItem		*GetSourceItem() const { return fSourceItem; };

	void			SetNeedsSave(bool needs);

	void			InitRename(BMessage* message);
	void			AbortRename();
	void			CommitRename();

private:
	SourceItem		*fSourceItem;
	bool			fFirstTimeRendered;
	bool			fNeedsSave;
	BRect			fTextRect;
	bool			fInitRename;
	BMessage*		fMessage;
	BTextControl	*fTextControl;

	void			_DrawIcon(BView* owner);
	void			_DrawTextWidget(BView* owner);
	void			_DestroyTextWidget();
};


#endif // PROJECT_ITEM_H
