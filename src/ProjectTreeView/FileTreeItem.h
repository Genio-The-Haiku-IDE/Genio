/*
 * Copyright 2023 Nexus6 <nexus6.haiku@icloud.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FILETREE_ITEM_H
#define FILETREE_ITEM_H

#include <Bitmap.h>
#include <Entry.h>
#include <Font.h>
#include <Path.h>
#include <StringItem.h>
#include <View.h>

class FileTreeItem : public BStringItem {
public:
						FileTreeItem();
						FileTreeItem(const entry_ref& ref, BOutlineListView *view);
						~FileTreeItem();

	status_t			InitCheck() const { return fInitStatus; }
	void				SetTo(const entry_ref& ref);

	entry_ref*			GetRef() const { return fRef; };
	
	BOutlineListView*	GetFileTreeView() const { return fFileTreeView; };
	
	FileTreeItem*		GetParentItem() const { return fParentItem; };
	void				SetParentItem(FileTreeItem *item) { fParentItem = item; };

	const char*			Text() const { return fRef->name; };
		
	void 				DrawItem(BView* owner, BRect bounds, bool complete);
	void 				Update(BView* owner, const BFont* font);

	bool				IsDirectory() const;

	void				SetActive(bool active) { fIsActive = active; }
	bool				IsActive() const { return fIsActive; }
	
	void				SetAsRoot(bool root = true) { fIsRoot = root; }
	bool				IsRoot() const { return fIsRoot; }
	
private:

	BBitmap*			fIcon;
	entry_ref*			fRef;
	BOutlineListView*	fFileTreeView;
	FileTreeItem*		fParentItem;
	bool				fFirstTimeRendered;
	status_t			fInitStatus;
	bool				fIsActive;
	bool				fIsRoot;
	
	// Derived class should not be able to set the Text manually
	void				SetText(const char* text);
	
};


#endif // FILETREE_ITEM_H
