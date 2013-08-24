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

std::vector<std::string> VCMIDirs::configPaths() const
{
	return std::vector<std::string>(1, dataPaths()[0] + "/config");
}

//FIXME: find way to at least decrease size of this ifdef (along with cleanup in CMake)
#if defined(_WIN32)

std::string VCMIDirs::userDataPath() const
{
	return dataPaths()[0];
}

std::string VCMIDirs::libraryPath() const
{
	return userDataPath();
}

std::string VCMIDirs::clientPath() const
{
	return userDataPath() + "\\" + "VCMI_client.exe";
}

std::string VCMIDirs::serverPath() const
{
	return userDataPath() + "\\" + "VCMI_server.exe";
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
	return userDataPath() + "\\" + "VCMI_client.exe";
}

std::string VCMIDirs::serverPath() const
{
	return userDataPath() + "\\" + "VCMI_server.exe";
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

std::string VCMIDirs::userDataPath() const
{
	if (getenv("HOME") != nullptr )
		return std::string(getenv("HOME")) + "/.vcmi";
	return ".";
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

std::vector<std::string> VCMIDirs::dataPaths() const
{
	return std::vector<std::string>(1, M_DATA_DIR);
}

std::string VCMIDirs::libraryName(std::string basename) const
{
	return "lib" + basename + ".so";
}

#endif
