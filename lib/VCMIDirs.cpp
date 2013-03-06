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
	boost::filesystem::create_directory(localPath());
	boost::filesystem::create_directory(localPath() + "/config");
	boost::filesystem::create_directory(localPath() + "/Games");
}

VCMIDirs & VCMIDirs::get()
{
	return VCMIDirsGlobal;
}

//FIXME: find way to at least decrease size of this ifdef (along with cleanup in CMake)
#if defined(_WIN32)

std::string VCMIDirs::localPath() const
{
	return dataPath();
}

std::string VCMIDirs::libraryPath() const
{
	return dataPath();
}

std::string VCMIDirs::serverPath() const
{
	return dataPath() + "\\" + "VCMI_server.exe";
}

std::string VCMIDirs::dataPath() const
{
	return ".";
}

std::string VCMIDirs::libraryName(std::string basename) const
{
	return basename + ".dll";
}

#elif defined(__APPLE__)

std::string VCMIDirs::localPath() const
{
	// This is Cocoa code that should be normally used to get path to Application Support folder but can't use it here for now...
	// NSArray* urls = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask];
	// UserPath = path([urls[0] path] + "/vcmi").string();

	// ...so here goes a bit of hardcode instead
	std::string home_dir = ".";
	if (getenv("HOME") != NULL )
		home_dir = getenv("HOME");

	return boost::filesystem::path(home_dir + "/Library/Application Support/vcmi").string();
}

std::string VCMIDirs::libraryPath() const
{
	return ".";
}

std::string VCMIDirs::serverPath() const
{
	return "./vcmiserver";
}

std::string VCMIDirs::dataPath() const
{
	return "../Data";
}

std::string VCMIDirs::libraryName(std::string basename) const
{
	return "lib" + basename + ".dylib";
}

#else

std::string VCMIDirs::localPath() const
{
	if (getenv("HOME") != NULL )
		return std::string(getenv("HOME")) + "/.vcmi";
	return ".";
}

std::string VCMIDirs::libraryPath() const
{
	return M_LIB_DIR;
}

std::string VCMIDirs::serverPath() const
{
	return std::string(M_BIN_DIR) + "/" + "vcmiserver";
}

std::string VCMIDirs::dataPath() const
{
	return M_DATA_DIR;
}

std::string VCMIDirs::libraryName(std::string basename) const
{
	return "lib" + basename + ".so";
}

#endif
