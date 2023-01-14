/*
 * Copyright 2022 D. Alfano
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

#include <Path.h>
#include <File.h>
#include <FindDirectory.h>

#define TYPE_COMPARE(t1,t2) (typeid(t1)==typeid(t2))

class GSettings : public BMessage {
public:
					GSettings(const BString& path, uint32 command);
					~GSettings();
	
	BString 		GetUserSettingsFolder();

	void			SetBool(const char *key, bool value);
	void			SetInt8(const char *key, int8 value);
	void			SetInt16(const char *key, int16 value);
	void			SetInt32(const char *key, int32 value);
	void			SetInt64(const char *key, int64 value);
	void			SetFloat(const char *key, float value);
	void			SetDouble(const char *key, double value);
	void			SetString(const char *key, const char *value);
	void			SetBString(const char* key, const BString& value);
	void			SetPoint(const char *key, BPoint value);
	void			SetRect(const char *key, BRect value);
	void			SetMessage(const char *key, const BMessage *value);
	void			SetFlat(const char *key, const BFlattenable *value);
	
private:
	BPath			fPath;
	BFile			fFile;
};

#endif // GSETTINGS_H
