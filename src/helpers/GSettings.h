/*
 * Copyright 2022 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Parts are taken from
 * TPreferences by Eric Shepard from Be Newsletter Issue 3-35, September 2, 1998
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 *
 * TODO: This we'll replace TPreferences when work on workspace is finished
 */

#ifndef GSETTINGS_H
#define GSETTINGS_H

#include <memory>

#include <Path.h>
#include <File.h>
#include <FindDirectory.h>

using namespace std;

class GSettings : public BMessage {
public:
						GSettings(const BString& folderPath, const BString& fileName, uint32 command);
						GSettings(const BString& fileName, uint32 command);
						~GSettings();

	void				Save();

	status_t			GetStatus() const { return fStatus; }

private:
	BPath				fPath;
	status_t			fStatus;
	bool				fSaved;
};

#endif // GSETTINGS_H
