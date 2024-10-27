/*
 * Copyright 2022 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Parts are taken from
 * TPreferences by Eric Shepard from Be Newsletter Issue 3-35, September 2, 1998
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 *
 * TODO: Get rid of this if possible
 */

#pragma once

#include <Path.h>

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
};
