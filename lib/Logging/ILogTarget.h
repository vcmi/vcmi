
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

class LogRecord;

/**
 * The interface log target is used by all log target implementations. It holds
 * the abstract method write which sub-classes should implement.
 */
class DLL_LINKAGE ILogTarget : public boost::noncopyable
{
public:
    /**
     * Writes a log record.
     *
     * @param record The log record to write.
     */
    virtual void write(const LogRecord & record) = 0;

    virtual ~ILogTarget() { };
};
