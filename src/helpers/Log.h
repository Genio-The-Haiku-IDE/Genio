/*
 * Copyright by Andrea Anzani 
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

#endif // LOG_H
