
/*
 * CBasicLogConfigurator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CLogger.h"

class CConsoleHandler;

/**
 * The basic log configurator reads log properties from the settings.json and
 * sets up the logging system. The file path of the log file can be specified
 * via the constructor.
 */
class DLL_LINKAGE CBasicLogConfigurator
{
public:
    /**
     * Constructor.
     *
     * @param fileName The file path of the log file.
     *
     * @throws std::runtime_error if the configuration has errors
     */
    CBasicLogConfigurator(const std::string & filePath, CConsoleHandler * console);

private:
    /**
     * Gets the log level enum from a string.
     *
     * @param level The log level string to parse.
     * @return the corresponding log level enum value
     *
     * @throws std::runtime_error if the log level is unknown
     */
    ELogLevel::ELogLevel getLogLevel(const std::string & level) const;

    /**
     * Gets the console text color enum from a string.
     *
     * @param colorName The color string to parse.
     * @return the corresponding console text color enum value
     *
     * @throws std::runtime_error if the log level is unknown
     */
    EConsoleTextColor::EConsoleTextColor getConsoleColor(const std::string & colorName) const;
};
