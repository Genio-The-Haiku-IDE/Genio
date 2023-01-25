//
// TPreferences
// Using BMessages to save user settings.
//
// Eric Shepherd
//
/*
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */

// from: Be Newsletter Issue 3-35, September 2, 1998

#ifndef T_PREFERENCES_H
#define T_PREFERENCES_H

#include <Path.h>
#include <File.h>
#include <FindDirectory.h>



class TPreferences : public BMessage {
public:
					TPreferences(const BString filename, const BString directory,
						uint32 command);
					~TPreferences();

	status_t		InitCheck(void);
	
	status_t		SetBool(const char *name, bool b);
	status_t		SetInt8(const char *name, int8 i);
	status_t		SetInt16(const char *name, int16 i);
	status_t		SetInt32(const char *name, int32 i);
	status_t		SetInt64(const char *name, int64 i);
	status_t		SetFloat(const char *name, float f);
	status_t		SetDouble(const char *name, double d);
	status_t		SetString(const char *name, const char *string);
	status_t		SetBString(const char* name, const BString& bstring);
	status_t		SetPoint(const char *name, BPoint p);
	status_t		SetRect(const char *name, BRect r);
	status_t		SetMessage(const char *name, const BMessage *message);
	status_t		SetFlat(const char *name, const BFlattenable *obj);
	
	
private:
	
	BPath			fPath;
	status_t		fStatus;
};

inline status_t TPreferences::InitCheck(void) {
	return fStatus;
}

#endif // T_PREFERENCES_H
