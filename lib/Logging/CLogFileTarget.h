
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
    ~CLogFileTarget();

    // Accessors

    const CLogFormatter & getFormatter() const;
    void setFormatter(const CLogFormatter & formatter);

    // Methods

    /**
     * Writes a log record to the log file.
     *
     * @param record The log record to write.
     */
    void write(const LogRecord & record);

private:
    // Data members

    std::ofstream file;
    CLogFormatter formatter;
    mutable boost::mutex mx;
};

