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

enum class EUserDirectory
{
	DATA,
	CACHE,
	CONFIG,
	LOGS,
	SAVES
};

class DLL_LINKAGE IVCMIDirs
{
public:
	// Path to user-specific data directory
	virtual boost::filesystem::path userDataPath() const = 0;

	// Path to "cache" directory, can be used for any non-essential files
	virtual boost::filesystem::path userCachePath() const = 0;

	// Path to writeable directory with user configs
	virtual boost::filesystem::path userConfigPath() const = 0;

	// Path to writeable directory to store log files
	virtual boost::filesystem::path userLogsPath() const;

	// Path to saved games
	virtual boost::filesystem::path userSavePath() const;
	boost::filesystem::path userPath(EUserDirectory directory) const;

	// Path to "extracted" directory, used to temporarily hold extracted Original H3 files
	virtual boost::filesystem::path userExtractedPath() const;

	// Paths to global system-wide data directories. First items have higher priority
	virtual std::vector<boost::filesystem::path> dataPaths() const = 0;

	// Full path to client executable, including name (e.g. /usr/bin/vcmiclient)
	virtual boost::filesystem::path clientPath() const = 0;

	// Full path to editor executable, including name (e.g. /usr/bin/vcmieditor)
	virtual boost::filesystem::path mapEditorPath() const = 0;

	// Full path to server executable, including name (e.g. /usr/bin/vcmiserver)
	virtual boost::filesystem::path serverPath() const = 0;

	// Path where vcmi binaries can be found
	virtual boost::filesystem::path binaryPath() const = 0;

	virtual std::string genHelpString() const;

	// Creates not existed, but required directories.
	// Updates directories what change name/path between versions.
	// Function called automatically.
	virtual void init();

	/// Changes a user directory and persists it if supported by the platform.
	virtual bool setUserPath(EUserDirectory directory, const boost::filesystem::path & path);
};

namespace VCMIDirs
{
	extern DLL_LINKAGE IVCMIDirs & get();

	/// Application ID as known to the OS: package name on Android, desktop file name on Linux
	extern DLL_LINKAGE const std::string & appIdentifier();
}
