
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
class JsonNode;

/// The class CBasicLogConfigurator reads log properties from settings.json and
/// sets up the logging system.
class DLL_LINKAGE CBasicLogConfigurator
{
public:
    CBasicLogConfigurator(const std::string & filePath, CConsoleHandler * console);

    /// Configures the logging system by parsing the logging settings. It adds the console target and the file target to the global logger.
    /// If the append parameter is true, the log file will be appended to. Otherwise the log file will be truncated.
    /// Throws std::runtime_error if the configuration has errors.
    void configure(bool appendToLogFile = true);

    /// Configures a default logging system by adding the console target and the file target to the global logger.
    /// If the append parameter is true, the log file will be appended to. Otherwise the log file will be truncated.
    void configureDefault(bool appendToLogFile = true);

private:
    ELogLevel::ELogLevel getLogLevel(const std::string & level) const;
    EConsoleTextColor::EConsoleTextColor getConsoleColor(const std::string & colorName) const;

    std::string filePath;
    CConsoleHandler * console;
};
