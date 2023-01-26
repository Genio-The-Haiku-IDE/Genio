/*
 * Copyright 2023 Your_Name 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SOURCE_FILE_H
#define SOURCE_FILE_H

#include "GenioCommon.h"
#include "GenioNamespace.h"
#include "ProjectFolder.h"

enum GENIO_SOURCE_ITEM_TYPE {
	G_FILE,
	G_FOLDER,
	G_PROJECT_FOLDER
};

class ProjectFolder;

class SourceFile {
public:
								SourceFile(BString const& path);
								~SourceFile();
								
	BString	const				Path() { return fPath; }
	BString	const				Name() { return fName; };
	GENIO_SOURCE_ITEM_TYPE		Type() { return fType; };
	
	ProjectFolder				*GetProjectFolder()	{ return fProjectFolder; }
								
protected:
	BString						fPath;
	BString						fName;
	GENIO_SOURCE_ITEM_TYPE		fType;
	ProjectFolder				*fProjectFolder;
};


#endif // SOURCE_FILE_H
