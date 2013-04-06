
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
 * The basic log configurator reads log properties from settings.json and
 * sets up the logging system.
 */
class DLL_LINKAGE CBasicLogConfigurator
{
public:
    /**
     * Constructor.
     *
     * @param filePath The file path of the log file.
     * @param console The console handler to log messages to the console. The handler should be initialized.
     *
     * @throws std::runtime_error if the configuration has errors
     */
    CBasicLogConfigurator(const std::string & filePath, CConsoleHandler * console);

private:
    // Methods

    ELogLevel::ELogLevel getLogLevel(const std::string & level) const;
    EConsoleTextColor::EConsoleTextColor getConsoleColor(const std::string & colorName) const;
};
