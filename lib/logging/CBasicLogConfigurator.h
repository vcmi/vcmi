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

VCMI_LIB_NAMESPACE_BEGIN

class CConsoleHandler;
class JsonNode;

/// The class CBasicLogConfigurator reads log properties from settings.json and
/// sets up the logging system. Make sure that you use the same configurator object to
/// initialize the whole logging system. If you don't do so, the log file will be overwritten/truncated.
class DLL_LINKAGE CBasicLogConfigurator
{
public:
	CBasicLogConfigurator(boost::filesystem::path filePath, CConsoleHandler * const console);

	/// Configures the logging system by parsing the logging settings. It adds the console target and the file target to the global logger.
	/// Doesn't throw, but logs on success or fault.
	void configure();

	/// Configures a default logging system by adding the console target and the file target to the global logger.
	void configureDefault();

	/// Removes all targets from the global logger.
	void deconfigure();


private:
	// Gets ELogLevel enum from string. (Should be moved to CLogger as a separate function?)
	// Throws: std::runtime_error
	static ELogLevel::ELogLevel getLogLevel(const std::string & level);
	// Gets EConsoleTextColor enum from strings. (Should be moved to CLogger as a separate function?)
	// Throws: std::runtime_error
	static EConsoleTextColor::EConsoleTextColor getConsoleColor(const std::string & colorName);

	boost::filesystem::path filePath;
	CConsoleHandler * console;
	bool appendToLogFile;
};

VCMI_LIB_NAMESPACE_END
