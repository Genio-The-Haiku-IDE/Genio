/*
 * Copyright 2022 D. Alfano
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


GSettings::GSettings(const BString& folderPath, const BString& fileName, uint32 command)
	: 
 	BMessage(command)
{
	status_t status;
	
	fPath.SetTo(folderPath);
	fPath.Append(fileName);		
	status = fFile.SetTo(fPath.Path(), B_READ_WRITE | B_CREATE_FILE);
	if (status != B_OK)
		throw BException("File system error: error while opening file", 0, status);
		
	status = Unflatten(&fFile);
	if (status != B_OK) {
		status = Flatten(&fFile);
		if (status != B_OK)
			throw BException("File system error: error while flattening file", 0, status);
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
	Flatten(&fFile);
}

void
GSettings::SetBool(const char *key, bool value) {
	status_t status = BMessage::SetBool(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetInt8(const char *key, int8 value) {
	status_t status = BMessage::SetInt8(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetInt16(const char *key, int16 value) {
	status_t status = BMessage::SetInt16(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetInt32(const char *key, int32 value) {
	status_t status = BMessage::SetInt32(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetInt64(const char *key, int64 value) {
	status_t status = BMessage::SetInt64(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetFloat(const char *key, float value) {
	status_t status = BMessage::SetFloat(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetDouble(const char *key, double value) {
	status_t status = BMessage::SetDouble(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetString(const char *key, const char *value) {
	status_t status = BMessage::SetString(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetBString(const char *key, const BString& value) {
	status_t status = BMessage::SetString(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetPoint(const char *key, BPoint value) {
	status_t status = BMessage::SetPoint(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}

void
GSettings::SetRect(const char *key, BRect value) {
	status_t status = BMessage::SetRect(key, value);
	if (status!=B_OK) 
		throw BException("GSettings: set value exception", 0, status);
	else
		Flatten(&fFile);
}