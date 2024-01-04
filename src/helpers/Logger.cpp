/*
 * Copyright 2017-2020, Andrew Lindesay .
 * Copyright 2023-2024 Genio
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Logger.h"

#include <cctype>
#include <stdarg.h>
#include <syslog.h>

#include "BeDC.h"

log_level Logger::sLevel = LOG_LEVEL_INFO;
int Logger::sDestination = LOGGER_DEST_STDOUT;

static BeDC sBeDC("Genio");

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
	_DoLog(LOG_LEVEL_INFO, logString);
}


/*static*/
void
Logger::LogFormat(log_level level, const char* fmtString, ...)
{
	// Prepend the log level
	char fullString[1024 + 4];
	snprintf(fullString, 4 + 1, "{%c} ", toupper(Logger::NameForLevel(level)[0]));

	// pass the offsetted array to vsnprintf
	char* logString = fullString + 4;
	va_list argp;
	::va_start(argp, fmtString);
	::vsnprintf(logString, sizeof(fullString) - 4, fmtString, argp);
	::va_end(argp);
	_DoLog(level, fullString);
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
	printf("Log level set to [%s] (%d)\n", Logger::NameForLevel(sLevel), sLevel);
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
	if (::strcasecmp(name, "off") == 0) {
		SetLevel(LOG_LEVEL_OFF);
	} else if (::strcasecmp(name, "info") == 0) {
		SetLevel(LOG_LEVEL_INFO);
	} else if (::strcasecmp(name, "debug") == 0) {
		SetLevel(LOG_LEVEL_DEBUG);
	} else if (::strcasecmp(name, "trace") == 0) {
		SetLevel(LOG_LEVEL_TRACE);
	} else if (::strcasecmp(name, "error") == 0) {
		SetLevel(LOG_LEVEL_ERROR);
	} else {
		return false;
	}

	return true;
}


/* static */
bool
Logger::SetLevelByChar(char level)
{
	switch (level) {
		case 'o':
			SetLevel(LOG_LEVEL_OFF);
			break;
		case 'e':
			SetLevel(LOG_LEVEL_ERROR);
			break;
		case 'i':
			SetLevel(LOG_LEVEL_INFO);
			break;
		case 'd':
			SetLevel(LOG_LEVEL_DEBUG);
			break;
		case 't':
			SetLevel(LOG_LEVEL_TRACE);
			break;
		default:
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
Logger::_DoLog(log_level level, const char* string)
{
	switch (sDestination) {
		case Logger::LOGGER_DEST_STDERR:
			::fprintf(stderr, "%s\n", string);
			break;
		case Logger::LOGGER_DEST_SYSLOG:
			::syslog(LOG_INFO|LOG_PID|LOG_CONS|LOG_USER, "Genio: %s", (const char* const)string);
			break;
		case Logger::LOGGER_DEST_BEDC:
			sBeDC.SendMessage(string, level == LOG_LEVEL_ERROR ? DC_ERROR : DC_MESSAGE);
			break;
		case Logger::LOGGER_DEST_STDOUT:
		default:
			::fprintf(stdout, "%s\n", string);
			break;
	}
}
