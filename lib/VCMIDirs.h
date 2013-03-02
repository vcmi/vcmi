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

	/// Path to local, user-specific directory (e.g. ~/.vcmi on *nix systems)
	std::string localPath() const;

	/// Path where vcmi libraries can be found (in AI and Scripting subdirectories)
	std::string libraryPath() const;

	/// Path to vcmiserver, including server name (e.g. /usr/bin/vcmiserver)
	std::string serverPath() const;

	/// Path to global system-wide data directory
	std::string dataPath() const;

	/// Returns system-specific name for dynamic libraries ("libStupidAI.so" or "StupidAI.dll")
	std::string libraryName(std::string basename) const;
};
