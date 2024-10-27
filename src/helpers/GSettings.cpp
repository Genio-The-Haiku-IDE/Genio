/*
 * Copyright 2022-2024 Nexus6 (nexus6.haiku@icloud.com)
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Parts are taken from
 * TPreferences by Eric Shepard from Be Newsletter Issue 3-35, September 2, 1998
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */
#include "GSettings.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <String.h>

#include "GenioNamespace.h"


GSettings::GSettings(const BString& folderPath, const BString& fileName, uint32 command)
	:
 	BMessage(command)
{
	fPath.SetTo(folderPath);
	fPath.Append(fileName);
	BFile file;
	fStatus = file.SetTo(fPath.Path(), B_READ_ONLY);
	if (fStatus == B_OK) {
		fStatus = Unflatten(&file);
		if (fStatus != B_OK) {
			// TODO: What to do here ?
		}	
	}
}


GSettings::GSettings(const BString& fileName, uint32 command)
	:
 	BMessage(command)
{
	fStatus = find_directory(B_USER_SETTINGS_DIRECTORY, &fPath);
	if (fStatus != B_OK) {
		return;
	}

	fPath.Append(GenioNames::kApplicationName);

	// fPath is absolute, so it doesn't matter that BDirectory isn't set
	fStatus = BDirectory().CreateDirectory(fPath.Path(), nullptr);

	fPath.Append(fileName);

	BFile file;
	fStatus = file.SetTo(fPath.Path(), B_READ_ONLY);
	if (fStatus == B_OK) {
		fStatus = Unflatten(&file);
	}
}


GSettings::~GSettings()
{
}


void
GSettings::Save()
{
	BFile file;
	fStatus = file.SetTo(fPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (fStatus == B_OK) {
		fStatus = Flatten(&file);
		if (fStatus != B_OK) {
			// TODO: What to do here ?
		}
	}
}
