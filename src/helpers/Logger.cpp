/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Logger.h"

#include <stdarg.h>
#include <syslog.h>

log_level Logger::sLevel = LOG_LEVEL_INFO;
int	Logger::sDestination = LOGGER_DEST_STDOUT;


/*static*/
void
Logger::SetDestination(int destination)
{
	sDestination = destination;
}


/*static*/
void
Logger::LogFormat(const char* fmtString, ...)
{
	char logString[1024];
	va_list argp;
	::va_start(argp, fmtString);
	::vsnprintf(logString, sizeof(logString), fmtString, argp);
	::va_end(argp);
	_DoLog(logString);
}


/*static*/
void
Logger::LogFormat(log_level level, const char* fmtString, ...)
{
	char fullString[1024 + 4];
	snprintf(fullString, 4 + 1, "{%c} ", toupper(Logger::NameForLevel(level)[0]));

	char* logString = fullString + 4;
	va_list argp;
	::va_start(argp, fmtString);
	::vsnprintf(logString, sizeof(fullString) - 4, fmtString, argp);
	::va_end(argp);
	_DoLog(fullString);
}


/*static*/
log_level
Logger::Level()
{
	return sLevel;
}


/*static*/
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


/*static*/
void
Logger::_DoLog(const char* string)
{
	switch (sDestination) {
		case Logger::LOGGER_DEST_STDERR:
			::fprintf(stderr, string); ::fprintf(stderr, "\n");
			break;
		/*case Logger::LOGGER_DEST_FILE:
			break;*/
		case Logger::LOGGER_DEST_SYSLOG:
			::syslog(LOG_INFO|LOG_PID|LOG_CONS|LOG_USER, "Genio: %s", (const char* const)string);
			break;
		case Logger::LOGGER_DEST_STDOUT:
		default:
			::fprintf(stdout, string); ::fprintf(stdout, "\n");
			break;
	}
}