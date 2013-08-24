#pragma once

#include "GameConstants.h"

/*
 * VCMIDirs.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Where to find the various VCMI files. This is mostly useful for linux. 
class DLL_LINKAGE VCMIDirs
{
public:
	VCMIDirs();

	/// get singleton instance
	static VCMIDirs & get();

	/// Path to user-specific data directory
	std::string userDataPath() const;

	/// Path to "cache" directory, can be used for any non-essential files
	std::string userCachePath() const;

	/// Path to writeable directory with user configs
	std::string userConfigPath() const;

	/// Path to saved games
	std::string userSavePath() const;

	/// Path to config directories, e.g. <data dir>/config. First items have higher priority
	std::vector<std::string> configPaths() const;

	/// Paths to global system-wide data directories. First items have higher priority
	std::vector<std::string> dataPaths() const;

	/// Full path to client executable, including server name (e.g. /usr/bin/vcmiclient)
	std::string clientPath() const;

	/// Full path to server executable, including server name (e.g. /usr/bin/vcmiserver)
	std::string serverPath() const;

	/// Path where vcmi libraries can be found (in AI and Scripting subdirectories)
	std::string libraryPath() const;

	/// Returns system-specific name for dynamic libraries ( StupidAI => "libStupidAI.so" or "StupidAI.dll")
	std::string libraryName(std::string basename) const;
};
