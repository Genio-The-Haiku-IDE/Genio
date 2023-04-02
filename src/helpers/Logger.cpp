/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Logger.h"


log_level Logger::sLevel = LOG_LEVEL_INFO;


log_level
Logger::Level()
{
	return sLevel;
}


void
Logger::SetLevel(log_level value)
{
	sLevel = value;
}


/*static*/
const char*
Logger::NameForLevel(log_level value)
{
	switch (value) {
		case LOG_LEVEL_OFF:
			return "off";
		case LOG_LEVEL_INFO:
			return "info";
		case LOG_LEVEL_DEBUG:
			return "debug";
		case LOG_LEVEL_TRACE:
			return "trace";
		case LOG_LEVEL_ERROR:
			return "error";
		default:
			return "?";
	}
}


/*static*/
bool
Logger::SetLevelByName(const char *name)
{
	if (strcmp(name, "off") == 0) {
		sLevel = LOG_LEVEL_OFF;
	} else if (strcmp(name, "info") == 0) {
		sLevel = LOG_LEVEL_INFO;
	} else if (strcmp(name, "debug") == 0) {
		sLevel = LOG_LEVEL_DEBUG;
	} else if (strcmp(name, "trace") == 0) {
		sLevel = LOG_LEVEL_TRACE;
	} else if (strcmp(name, "error") == 0) {
		sLevel = LOG_LEVEL_ERROR;
	} else {
		return false;
	}

	return true;
}


/*static*/
bool
Logger::IsLevelEnabled(log_level value)
{
	return sLevel >= value;
}


/*static*/
bool
Logger::IsInfoEnabled()
{
	return IsLevelEnabled(LOG_LEVEL_INFO);
}


/*static*/
bool
Logger::IsDebugEnabled()
{
	return IsLevelEnabled(LOG_LEVEL_DEBUG);
}


/*static*/
bool
Logger::IsTraceEnabled()
{
	return IsLevelEnabled(LOG_LEVEL_TRACE);
}


/*static*/
bool
Logger::IsErrorEnabled()
{
	return IsLevelEnabled(LOG_LEVEL_ERROR);
}
