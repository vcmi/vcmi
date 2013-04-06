
/*
 * LogRecord.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CLogger.h"

/**
 * The log records holds the log message and additional logging information.
 */
struct DLL_LINKAGE LogRecord
{
    LogRecord(const CLoggerDomain & domain, ELogLevel::ELogLevel level, const std::string & message)
        : domain(domain), level(level), message(message), timeStamp(boost::posix_time::second_clock::local_time()), threadId(boost::this_thread::get_id())
    {

    }

    /** The logger domain. */
    CLoggerDomain domain;

    /** The log level. */
    ELogLevel::ELogLevel level;

    /** The message. */
    std::string message;

    /** The time when the message was created. */
    boost::posix_time::ptime timeStamp;

    /** The thread id. */
    boost::thread::id threadId;
};
