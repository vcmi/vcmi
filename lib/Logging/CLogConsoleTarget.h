
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

    // Methods

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
    // Data members

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

    // Accessors

    bool isColoredOutputEnabled() const;
    void setColoredOutputEnabled(bool coloredOutputEnabled);

    ELogLevel::ELogLevel getThreshold() const;
    void setThreshold(ELogLevel::ELogLevel threshold);

    const CLogFormatter & getFormatter() const;
    void setFormatter(const CLogFormatter & formatter);

    const CColorMapping & getColorMapping() const;
    void setColorMapping(const CColorMapping & colorMapping);

    // Methods

    /**
     * Writes a log record to the console.
     *
     * @param record The log record to write.
     */
    void write(const LogRecord & record);

private:
    // Data members

    CConsoleHandler * console;
    ELogLevel::ELogLevel threshold;
    bool coloredOutputEnabled;
    CLogFormatter formatter;
    CColorMapping colorMapping;
    mutable boost::mutex mx;
};
