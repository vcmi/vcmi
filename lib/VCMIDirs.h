/*
* VCMIDirs.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

class DLL_LINKAGE IVCMIDirs
{
	public:
		// Path to user-specific data directory
		virtual boost::filesystem::path userDataPath() const = 0;

		// Path to "cache" directory, can be used for any non-essential files
		virtual boost::filesystem::path userCachePath() const = 0;

		// Path to writeable directory with user configs
		virtual boost::filesystem::path userConfigPath() const = 0;

		// Path to saved games
		virtual boost::filesystem::path userSavePath() const;

		// Paths to global system-wide data directories. First items have higher priority
		virtual std::vector<boost::filesystem::path> dataPaths() const = 0;

		// Full path to client executable, including server name (e.g. /usr/bin/vcmiclient)
		virtual boost::filesystem::path clientPath() const = 0;

		// Full path to server executable, including server name (e.g. /usr/bin/vcmiserver)
		virtual boost::filesystem::path serverPath() const = 0;

		// Path where vcmi libraries can be found (in AI and Scripting subdirectories)
		virtual boost::filesystem::path libraryPath() const = 0;

		// Path where vcmi binaries can be found
		virtual boost::filesystem::path binaryPath() const = 0;

		// Returns system-specific name for dynamic libraries ( StupidAI => "libStupidAI.so" or "StupidAI.dll")
		virtual std::string libraryName(const std::string& basename) const = 0;
		// virtual std::string libraryName(const char* basename) const = 0; ?
		// virtual std::string libraryName(std::string&& basename) const = 0;?

		virtual std::string genHelpString() const = 0;
		
		// Creates not existed, but required directories.
		// Updates directories what change name/path between versions.
		// Function called automatically.
		virtual void init();
};

namespace VCMIDirs
{
	extern DLL_LINKAGE const IVCMIDirs& get();
}
