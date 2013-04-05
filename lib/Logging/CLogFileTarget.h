
/*
 * CLogFileTarget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ILogTarget.h"
#include "CLogFormatter.h"

/**
 * The log file target is a logging target which writes messages to a log file.
 */
class DLL_LINKAGE CLogFileTarget : public ILogTarget
{
public:
    /**
     * Constructor.
     *
     * @param filePath The file path of the log file.
     */
    explicit CLogFileTarget(const std::string & filePath);

    /**
     * Destructor.
     */
    ~CLogFileTarget();

    /**
     * Writes a log record.
     *
     * @param record The log record to write.
     */
    void write(const LogRecord & record);

    /**
     * Gets the log formatter.
     *
     * @return The log formatter.
     */
    const CLogFormatter & getFormatter() const;

    /**
     * Sets the log formatter.
     *
     * @param formatter The log formatter.
     */
    void setFormatter(const CLogFormatter & formatter);

private:
    /** The output file stream. */
    std::ofstream file;

    /** The log formatter to log messages. */
    CLogFormatter formatter;

    /** The shared mutex for providing synchronous thread-safe access to the log file target. */
    mutable boost::mutex mx;
};

