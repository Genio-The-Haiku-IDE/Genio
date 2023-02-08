/*
 * Copyright 2022 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 * 
 * Parts are taken from 
 * TPreferences by Eric Shepard from Be Newsletter Issue 3-35, September 2, 1998
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */

#include <memory>
#include <typeinfo>

#include <Message.h>
#include <Messenger.h>
#include <Directory.h>
#include <String.h>
#include <NodeInfo.h>

#include "GSettings.h"
#include "exceptions/Exceptions.h"

GSettings::GSettings(const BString& folderPath, const BString& fileName, uint32 command)
	: 
 	BMessage(command)
{
	BFile fFile;
	
	fPath.SetTo(folderPath);
	fPath.Append(fileName);		
	fStatus = fFile.SetTo(fPath.Path(), B_READ_ONLY);
	if (fStatus==B_OK) {
		fStatus = Unflatten(&fFile);
		if (fStatus!=B_OK)
			throw BException("File system error: can't read settings from file", 0, fStatus);
	} else {
		throw BException("File system error: can't open settings file", 0, fStatus);
	}
}

BString
GSettings::GetUserSettingsFolder()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK) {
		throw BException("File system error: can't find directory", 0, status);
	}
	return path.Path();
}

GSettings::~GSettings() {
	Save();
}

void
GSettings::Save() {
	BFile file;
	file.SetTo(fPath.Path(), B_WRITE_ONLY | B_ERASE_FILE);
	fStatus = Flatten(&file);
}