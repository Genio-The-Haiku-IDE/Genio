/*
 * Copyright by Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LOG_H
#define LOG_H

#include "Logger.h"

#define LogFatal(M...) HDFATAL(M)
#define LogError(M...) HDLOG(LOG_LEVEL_ERROR, M)
#define LogInfo(M...)  HDLOG(LOG_LEVEL_INFO, M)
#define LogDebug(M...) HDLOG(LOG_LEVEL_DEBUG, M)
#define LogTrace(M...) HDLOG(LOG_LEVEL_TRACE, M)

//Function prepending the calling method/function
#define LogFatalF(S, M...) LogFatal("%s " S, __PRETTY_FUNCTION__, M)
#define LogErrorF(S, M...) LogError("%s " S, __PRETTY_FUNCTION__, M)
#define LogInfoF(S, M...)  LogInfo("%s " S, __PRETTY_FUNCTION__, M)
#define LogDebugF(S, M...) LogDebug("%s " S, __PRETTY_FUNCTION__, M)
#define LogTraceF(S, M...) LogTrace("%s " S, __PRETTY_FUNCTION__, M)

#endif // LOG_H
