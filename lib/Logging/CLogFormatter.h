
/*
 * CLogFormatter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

struct LogRecord;

/**
 * The log formatter formats log records.
 *
 * There are several pattern characters which can be used to format a log record:
 * %d = Date/Time
 * %l = Log level
 * %n = Logger name
 * %t = Thread ID
 * %m = Message
 */
class DLL_LINKAGE CLogFormatter
{
public:
    CLogFormatter();

    /**
     * Constructor.
     *
     * @param pattern The pattern to format the log record with.
     */
    CLogFormatter(const std::string & pattern);

    // Accessors

    void setPattern(const std::string & pattern);
    const std::string & getPattern() const;

    // Methods

    /**
     * Formats a log record.
     *
     * @param record The log record to format.
     * @return the formatted log record as a string
     */
    std::string format(const LogRecord & record) const;


private:
    // Data members

    std::string pattern;
};
