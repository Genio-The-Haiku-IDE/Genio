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

#include "FileTreeView.h"

class FileTreeView;

class FileTreeItem : public BStringItem {
public:
						FileTreeItem();
						FileTreeItem(const entry_ref& ref, BOutlineListView *view);
						~FileTreeItem();

	status_t			InitCheck() const { return fInitStatus; }
	void				SetTo(const entry_ref& ref);

	entry_ref			GetRef() const { return fRef; };
	BEntry*				GetEntry() const { return fEntry; };
	BPath*				GetPath() const { return fPath; };
	BString*			GetStringPath() const { return fStringPath; };
	
	BOutlineListView*	GetFileTreeView() const { return fFileTreeView; };
	void				SetFileTreeView(BOutlineListView *view) { fFileTreeView = view; };
	
	FileTreeItem*		GetParentItem() const { return fParentItem; };
	void				SetParentItem(FileTreeItem *item);

	const char*			Text() const { return fRef.name; };
		
	void 				DrawItem(BView* owner, BRect bounds, bool complete);
	void 				Update(BView* owner, const BFont* font);

	bool				IsDirectory() const { return fEntry->IsDirectory(); };
	bool				IsScanned() const { return fIsScanned; };

	// void				SetActive(bool active) { fIsActive = active; }
	// bool				IsActive() const { return fIsActive; }
	// bool				IsRoot() const;
	
private:

	BBitmap*			fIcon;
	entry_ref			fRef;
	BEntry*				fEntry;
	BPath*				fPath;
	BString*			fStringPath;
	BOutlineListView*	fFileTreeView;
	FileTreeItem*		fParentItem;
	bool				fFirstTimeRendered;
	bool				fIsScanned;
	status_t			fInitStatus;
	
	// Derived class should not be able to set the Text manually
	void				SetText(const char* text);
	
};


#endif // FILETREE_ITEM_H
