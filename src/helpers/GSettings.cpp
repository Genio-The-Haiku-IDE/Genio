/*
 * Copyright 2022 Nexus6 ()
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Parts are taken from
 * TPreferences by Eric Shepard from Be Newsletter Issue 3-35, September 2, 1998
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */
#include "GSettings.h"

#include <Directory.h>
#include <Message.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <String.h>


GSettings::GSettings(const BString& folderPath, const BString& fileName, uint32 command)
	:
 	BMessage(command)
{
	BFile file;

	fPath.SetTo(folderPath);
	fPath.Append(fileName);
	fStatus = file.SetTo(fPath.Path(), B_READ_ONLY);
	if (fStatus == B_OK) {
		fStatus = Unflatten(&file);
		if (fStatus != B_OK)
			fSaved = false;
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
		if (fStatus == B_OK)
			fSaved = true;
	}
}
