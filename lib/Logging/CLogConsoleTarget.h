
/*
 * CLogConsoleTarget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ILogTarget.h"
#include "CLogger.h"
#include "CLogFormatter.h"
#include "CConsoleHandler.h"

struct LogRecord;

/**
 * The color mapping maps a logger name and a level to a specific color.
 */
class DLL_LINKAGE CColorMapping
{
public:
    /**
     * Constructor. There are default color mappings for the root logger, which child loggers inherit if not overriden.
     */
    CColorMapping();

    /**
     * Sets a console text color for a logger name and a level.
     *
     * @param domain The domain of the logger.
     * @param level The logger level.
     * @param color The console text color to use as the mapping.
     */
    void setColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level, EConsoleTextColor::EConsoleTextColor color);

    /**
     * Gets a console text color for a logger name and a level.
     *
     * @param domain The domain of the logger.
     * @param level The logger level.
     * @return the console text color which has been applied for the mapping
     */
    EConsoleTextColor::EConsoleTextColor getColorFor(const CLoggerDomain & domain, ELogLevel::ELogLevel level) const;

private:
    /** The color mapping, 1. Key: Logger domain, 2. Key: Level, Value: Color. */
    std::map<std::string, std::map<ELogLevel::ELogLevel, EConsoleTextColor::EConsoleTextColor> > map;
};

/**
 * The console target is a logging target which writes message to the console.
 */
class DLL_LINKAGE CLogConsoleTarget : public ILogTarget
{
public:
    /**
     * Constructor.
     *
     * @param console Optional. The console handler which is used to output messages to the console.
     */
    explicit CLogConsoleTarget(CConsoleHandler * console);

    /**
     * Writes a log record.
     *
     * @param record The log record to write.
     */
    void write(const LogRecord & record);

    /**
     * True if colored output is enabled.
     *
     * @return true if colored output is enabled, false if not
     */
    bool isColoredOutputEnabled() const;

    /**
     * Sets the colored output flag.
     *
     * @param coloredOutput True if the log target should write colored messages to the console, false if not.
     */
    void setColoredOutputEnabled(bool coloredOutputEnabled);

    /**
     * Gets the threshold log level.
     *
     * @return the threshold log level
     */
    ELogLevel::ELogLevel getThreshold() const;

    /**
     * Sets the threshold log level.
     *
     * @param threshold The threshold log level.
     */
    void setThreshold(ELogLevel::ELogLevel threshold);

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

    /**
     * Gets the color mapping.
     */
    const CColorMapping & getColorMapping() const;

    /**
     * Sets the color mapping.
     *
     * @param colorMapping The color mapping.
     */
    void setColorMapping(const CColorMapping & colorMapping);

private:
    /** The console handler which is used to output messages to the console. */
    CConsoleHandler * console;

    /** The threshold log level. */
    ELogLevel::ELogLevel threshold;

    /** Flag whether colored console output is enabled. */
    bool coloredOutputEnabled;

    /** The log formatter to log messages. */
    CLogFormatter formatter;

    /** The color mapping which maps a logger and a log level to a color. */
    CColorMapping colorMapping;

    /** The shared mutex for providing synchronous thread-safe access to the log console target. */
    mutable boost::mutex mx;
};
