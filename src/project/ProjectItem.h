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

	SourceItem		*GetSourceItem() const { return fSourceItem; };

	void			SetNeedsSave(bool needs);
	void			SetOpenedInEditor(bool open);

	void			InitRename(BMessage* message);
	void			AbortRename();
	void			CommitRename();

	bool			HasToolTip() const { return !fToolTipText.IsEmpty(); };
	void			SetToolTipText(const char *text) { fToolTipText = text; }
	const char*		GetToolTipText() const { return fToolTipText.String(); }

private:
	SourceItem		*fSourceItem;
	bool			fFirstTimeRendered;
	bool			fNeedsSave;
	bool			fOpenedInEditor;
	bool			fInitRename;
	BMessage*		fMessage;
	BTextControl	*fTextControl;
	BString			fPrimaryText;
	BString			fSecondaryText;
	BString			fToolTipText;

	void			_DrawText(BView* owner, const BPoint& textPoint);
	void			_DrawTextWidget(BView* owner, const BRect& textRect);
	void			_DestroyTextWidget();
};


#endif // PROJECT_ITEM_H
