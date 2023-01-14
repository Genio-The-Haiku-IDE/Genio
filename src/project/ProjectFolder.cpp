/*
 * Copyright 2018, Your Name 
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdexcept>

#include "ProjectFolder.h"

SourceFile::SourceFile(const char *path) 
{
	fPath = path;
}

SourceFile::~SourceFile()
{
}

BString	
SourceFile::GetPath()
{
	return fPath;
}

ProjectFolder *
SourceFile::GetProjectFolder()
{ 
	return fProjectFolder;
}


ProjectFolder::ProjectFolder(BString const& name)
{
	fName = name;
}

ProjectFolder::~ProjectFolder()
{
}

template <class T>
T							
ProjectFolder::GetConfig(GENIO_SETTINGS_KEY key) {
}

template <class T>
void						
ProjectFolder::SetConfig(GENIO_SETTINGS_KEY key, T value) {
}