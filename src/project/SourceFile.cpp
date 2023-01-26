/*
 * Copyright 2023 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Directory.h>
#include <Entry.h> 

#include "SourceFile.h"

SourceFile::SourceFile(BString const& path) 
{
	fPath = path;
	BPath _path(path);
	fName = _path.Leaf();
	
	BEntry entry(path);
	if (entry.IsDirectory())
		fType = G_FOLDER;
	else
		fType = G_FILE;
}

SourceFile::~SourceFile()
{
}

