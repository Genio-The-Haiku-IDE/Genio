/*
 * Copyright 2023 Your_Name 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PROJECT_ITEM_H
#define PROJECT_ITEM_H

#include <Font.h>
#include <StringItem.h>
#include <View.h>
#include <Bitmap.h>

class SourceItem;

class ProjectItem : public BStringItem {
public:
					ProjectItem(SourceItem *sourceFile);
					~ProjectItem();
						
	void 			DrawItem(BView* owner, BRect bounds, bool complete);
	void 			Update(BView* owner, const BFont* font);
	BRect 			GetTextRect() { return fTextRect; }

	SourceItem		*GetSourceItem() const { return fSourceItem; };
	
private:
	SourceItem		*fSourceItem;
	bool			firstTimeRendered = true;
	BRect			fTextRect;
};


#endif // PROJECT_ITEM_H
