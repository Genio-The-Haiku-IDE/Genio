/*
 * Copyright 2022 Nexus6
 * All rights reserved. Distributed under the terms of the MIT license.
 * 
 * Parts are taken from 
 * TPreferences by Eric Shepard from Be Newsletter Issue 3-35, September 2, 1998
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */

#include <Message.h>
#include <Messenger.h>
#include <Directory.h>
#include <String.h>
#include <NodeInfo.h>

#include <typeinfo>

#include "GSettings.h"
#include "exceptions/Exceptions.h"


GSettings::GSettings(const BString& filepath, uint32 command)
	: 
 	BMessage(command)
{
	status_t status;

	fPath.Append(filepath);		
	status = fFile.SetTo(fPath.Path(), B_READ_WRITE | B_CREATE_FILE);
	if (status != B_OK)
		throw BException("File system error: error while opening file", 0, status);
		
	status = Unflatten(&fFile);
	if (status != B_OK)
		throw BException("File system error: error while unflattening file", 0, status);
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
	Flatten(&fFile);
}

void
GSettings::SetBool(const char *key, bool value) {
	status_t status;
	
	if (HasBool(key)) {
		status = ReplaceBool(key, 0, value);
	} else {
		status = AddBool(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetInt8(const char *key, int8 value) {
	status_t status;
	
	if (HasInt8(key)) {
		status = ReplaceInt8(key, 0, value);
	} else {
		status = AddInt8(key, value);
	}
	
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetInt16(const char *key, int16 value) {
	status_t status;
	
	if (HasInt16(key)) {
		status = ReplaceInt16(key, 0, value);
	} else {
		status = AddInt16(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetInt32(const char *key, int32 value) {
	status_t status;
	
	if (HasInt32(key)) {
		status = ReplaceInt32(key, 0, value);
	} else {
		status = AddInt32(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetInt64(const char *key, int64 value) {
	status_t status;
	
	if (HasInt64(key)) {
		status = ReplaceInt64(key, 0, value);
	} else {
		status = AddInt64(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetFloat(const char *key, float value) {
	status_t status;
	
	if (HasFloat(key)) {
		status = ReplaceFloat(key, 0, value);
	} else {
		status = AddFloat(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetDouble(const char *key, double value) {
	status_t status;
	
	if (HasDouble(key)) {
		status = ReplaceDouble(key, 0, value);
	} else {
		status = AddDouble(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetString(const char *key, const char *value) {
	status_t status;
	
	if (HasString(key)) {
		status = ReplaceString(key, 0, value);
	} else {
		status = AddString(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetBString(const char *key, const BString& value) {
	status_t status;
	
	if (HasString(key)) {
		status = ReplaceString(key, 0, value);
	} else {
		status = AddString(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetPoint(const char *key, BPoint value) {
	status_t status;
	
	if (HasPoint(key)) {
		status = ReplacePoint(key, 0, value);
	} else {
		status = AddPoint(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetRect(const char *key, BRect value) {
	status_t status;
	
	if (HasRect(key)) {
		status = ReplaceRect(key, 0, value);
	} else {
		status = AddRect(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetMessage(const char *key, const BMessage *value) {
	status_t status;
	
	if (HasMessage(key)) {
		status = ReplaceMessage(key, 0, value);
	} else {
		status = AddMessage(key, value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetFlat(const char *key, const BFlattenable *value) {
	status_t status;
	
	if (HasFlat(key, value)) {
		status = ReplaceFlat(key, 0, (BFlattenable *)value);
	} else {
		status = AddFlat(key, (BFlattenable *)value);
	}

	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}