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

<<<<<<< HEAD
=======
#include "FileTreeView.h"

class FileTreeView;

>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
class FileTreeItem : public BStringItem {
public:
						FileTreeItem();
						FileTreeItem(const entry_ref& ref, BOutlineListView *view);
						~FileTreeItem();

	status_t			InitCheck() const { return fInitStatus; }
	void				SetTo(const entry_ref& ref);

<<<<<<< HEAD
	entry_ref*			GetRef() const { return fRef; };
	
	BOutlineListView*	GetFileTreeView() const { return fFileTreeView; };
	
	FileTreeItem*		GetParentItem() const { return fParentItem; };
	void				SetParentItem(FileTreeItem *item) { fParentItem = item; };

	const char*			Text() const { return fRef->name; };
=======
	entry_ref			GetRef() const { return fRef; };
	BEntry*				GetEntry() const { return fEntry; };
	BPath*				GetPath() const { return fPath; };
	BString*			GetStringPath() const { return fStringPath; };
	
	BOutlineListView*	GetFileTreeView() const { return fFileTreeView; };
	void				SetFileTreeView(BOutlineListView *view) { fFileTreeView = view; };
	
	FileTreeItem*		GetParentItem() const { return fParentItem; };
	void				SetParentItem(FileTreeItem *item);

	const char*			Text() const { return fRef.name; };
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
		
	void 				DrawItem(BView* owner, BRect bounds, bool complete);
	void 				Update(BView* owner, const BFont* font);

<<<<<<< HEAD
	bool				IsDirectory() const;

	void				SetActive(bool active) { fIsActive = active; }
	bool				IsActive() const { return fIsActive; }
	
	void				SetAsRoot(bool root = true) { fIsRoot = root; }
	bool				IsRoot() const { return fIsRoot; }
=======
	bool				IsDirectory() const { return fEntry->IsDirectory(); };
	bool				IsScanned() const { return fIsScanned; };

	// void				SetActive(bool active) { fIsActive = active; }
	// bool				IsActive() const { return fIsActive; }
	// bool				IsRoot() const;
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
	
private:

	BBitmap*			fIcon;
<<<<<<< HEAD
	entry_ref*			fRef;
	BOutlineListView*	fFileTreeView;
	FileTreeItem*		fParentItem;
	bool				fFirstTimeRendered;
	status_t			fInitStatus;
	bool				fIsActive;
	bool				fIsRoot;
=======
	entry_ref			fRef;
	BEntry*				fEntry;
	BPath*				fPath;
	BString*			fStringPath;
	BOutlineListView*	fFileTreeView;
	FileTreeItem*		fParentItem;
	bool				fFirstTimeRendered;
	bool				fIsScanned;
	status_t			fInitStatus;
>>>>>>> 4badbb7 (_ScanThread is run as an independent thread for each project.)
	
	// Derived class should not be able to set the Text manually
	void				SetText(const char* text);
	
};


#endif // FILETREE_ITEM_H
