
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

#include "../CConsoleHandler.h"

class CConsoleHandler;
class JsonNode;

/// The class CBasicLogConfigurator reads log properties from settings.json and
/// sets up the logging system. Make sure that you use the same configurator object to
/// initialize the whole logging system. If you don't do so, the log file will be overwritten/truncated.
class DLL_LINKAGE CBasicLogConfigurator
{
public:
    CBasicLogConfigurator(const std::string & filePath, CConsoleHandler * console);

    /// Configures the logging system by parsing the logging settings. It adds the console target and the file target to the global logger.
    /// Doesn't throw, but logs on success or fault.
    void configure();

    /// Configures a default logging system by adding the console target and the file target to the global logger.
    void configureDefault();

private:
    ELogLevel::ELogLevel getLogLevel(const std::string & level) const;
    EConsoleTextColor::EConsoleTextColor getConsoleColor(const std::string & colorName) const;

    std::string filePath;
    CConsoleHandler * console;
    bool appendToLogFile;
};
