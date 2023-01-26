/*
 * Copyright 2023, Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <OutlineListView.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include <stdexcept>

#include "ProjectFolder.h"


SourceItem::SourceItem(BString const& path) 
{
	fPath = path;
	BPath _path(path);
	fName = _path.Leaf();
	
	BEntry entry(path);
	if (entry.IsDirectory())
		fType = SourceItemType::FolderItem;
	else
		fType = SourceItemType::FileItem;
}

SourceItem::~SourceItem()
{
}

ProjectFolder::ProjectFolder(BString const& path)
	: SourceItem(path)
{	
	fType = SourceItemType::ProjectFolderItem;
	fProjectFolder = this;
	
	// _ScanFolder(fPath);
}

ProjectFolder::~ProjectFolder()
{
}

status_t
ProjectFolder::Open()
{
	return B_OK;
}