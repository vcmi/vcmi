#include "StdInc.h"
#include "VCMIDirs.h"

/*
 * VCMIDirs.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static VCMIDirs VCMIDirsGlobal;

VCMIDirs::VCMIDirs()
{
	// initialize local directory and create folders to which VCMI needs write access
	boost::filesystem::create_directory(userDataPath());
	boost::filesystem::create_directory(userCachePath());
	boost::filesystem::create_directory(userConfigPath());
	boost::filesystem::create_directory(userSavePath());
}

VCMIDirs & VCMIDirs::get()
{
	return VCMIDirsGlobal;
}

//FIXME: find way to at least decrease size of this ifdef (along with cleanup in CMake)
#if defined(_WIN32)

std::string VCMIDirs::userCachePath() const
{
	return userDataPath();
}

std::string VCMIDirs::userConfigPath() const
{
	return userDataPath() + "/config";
}

std::string VCMIDirs::userSavePath() const
{
	return userDataPath() + "/Games";
}

std::string VCMIDirs::userDataPath() const
{
	const std::string homeDir = std::getenv("userprofile");
	return homeDir + "\\vcmi";
	//return dataPaths()[0];
}

std::string VCMIDirs::libraryPath() const
{
	return ".";
}

std::string VCMIDirs::clientPath() const
{
	return libraryPath() + "\\" + "VCMI_client.exe";
}

std::string VCMIDirs::serverPath() const
{
	return libraryPath() + "\\" + "VCMI_server.exe";
}

std::vector<std::string> VCMIDirs::dataPaths() const
{
	return std::vector<std::string>(1, ".");
}

std::string VCMIDirs::libraryName(std::string basename) const
{
	return basename + ".dll";
}

#elif defined(__APPLE__)

std::string VCMIDirs::userCachePath() const
{
	return userDataPath();
}

std::string VCMIDirs::userConfigPath() const
{
	return userDataPath() + "/config";
}

std::string VCMIDirs::userSavePath() const
{
	return userDataPath() + "/Games";
}

std::string VCMIDirs::userDataPath() const
{
	// This is Cocoa code that should be normally used to get path to Application Support folder but can't use it here for now...
	// NSArray* urls = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask];
	// UserPath = path([urls[0] path] + "/vcmi").string();

	// ...so here goes a bit of hardcode instead
	std::string home_dir = ".";
	if (getenv("HOME") != nullptr )
		home_dir = getenv("HOME");

	return boost::filesystem::path(home_dir + "/Library/Application Support/vcmi").string();
}

std::string VCMIDirs::libraryPath() const
{
	return ".";
}

std::string VCMIDirs::clientPath() const
{
	return "./vcmiclient";
}

std::string VCMIDirs::serverPath() const
{
	return "./vcmiserver";
}

std::vector<std::string> VCMIDirs::dataPaths() const
{
	return std::vector<std::string>(1, "../Data");
}

std::string VCMIDirs::libraryName(std::string basename) const
{
	return "lib" + basename + ".dylib";
}

#else

std::string VCMIDirs::libraryName(std::string basename) const
{
	return "lib" + basename + ".so";
}

std::string VCMIDirs::libraryPath() const
{
	return M_LIB_DIR;
}

std::string VCMIDirs::clientPath() const
{
	return std::string(M_BIN_DIR) + "/" + "vcmiclient";
}

std::string VCMIDirs::serverPath() const
{
	return std::string(M_BIN_DIR) + "/" + "vcmiserver";
}

// $XDG_DATA_HOME, default: $HOME/.local/share
std::string VCMIDirs::userDataPath() const
{
	if (getenv("XDG_DATA_HOME") != nullptr )
		return std::string(getenv("XDG_DATA_HOME")) + "/vcmi";
	if (getenv("HOME") != nullptr )
		return std::string(getenv("HOME")) + "/.local/share" + "/vcmi";
	return ".";
}

std::string VCMIDirs::userSavePath() const
{
	return userDataPath() + "/Saves";
}

// $XDG_CACHE_HOME, default: $HOME/.cache
std::string VCMIDirs::userCachePath() const
{
	if (getenv("XDG_CACHE_HOME") != nullptr )
		return std::string(getenv("XDG_CACHE_HOME")) + "/vcmi";
	if (getenv("HOME") != nullptr )
		return std::string(getenv("HOME")) + "/.cache" + "/vcmi";
	return ".";
}

// $XDG_CONFIG_HOME, default: $HOME/.config
std::string VCMIDirs::userConfigPath() const
{
	if (getenv("XDG_CONFIG_HOME") != nullptr )
		return std::string(getenv("XDG_CONFIG_HOME")) + "/vcmi";
	if (getenv("HOME") != nullptr )
		return std::string(getenv("HOME")) + "/.config" + "/vcmi";
	return ".";
}

// $XDG_DATA_DIRS, default: /usr/local/share/:/usr/share/
std::vector<std::string> VCMIDirs::dataPaths() const
{
	// construct list in reverse.
	// in specification first directory has highest priority
	// in vcmi fs last directory has highest priority

	std::vector<std::string> ret;

	if (getenv("HOME") != nullptr ) // compatibility, should be removed after 0.96
		ret.push_back(std::string(getenv("HOME")) + "/.vcmi");
	ret.push_back(M_DATA_DIR);

	if (getenv("XDG_DATA_DIRS") != nullptr)
	{
		std::string dataDirsEnv = getenv("XDG_DATA_DIRS");
		std::vector<std::string> dataDirs;
		boost::split(dataDirs, dataDirsEnv, boost::is_any_of(":"));
		for (auto & entry : boost::adaptors::reverse(dataDirs))
			ret.push_back(entry + "/vcmi");
	}
	else
	{
		ret.push_back("/usr/share/");
		ret.push_back("/usr/local/share/");
	}
	return ret;
}

#endif
