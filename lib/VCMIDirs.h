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

// Boost
#include <boost/filesystem/path.hpp>

// STL C++
#include <string>
#include <vector>

// TODO: Remove _VCMIDirs
class _VCMIDirs;

// Where to find the various VCMI files. This is mostly useful for linux. 
namespace VCMIDirs
{
	namespace detail
	{
		extern DLL_LINKAGE boost::filesystem::path g_user_data_path;
		extern DLL_LINKAGE boost::filesystem::path g_user_cache_path;
		extern DLL_LINKAGE boost::filesystem::path g_user_config_path;
		extern DLL_LINKAGE boost::filesystem::path g_user_save_path;

		extern DLL_LINKAGE boost::filesystem::path g_library_path;
		extern DLL_LINKAGE boost::filesystem::path g_client_path;
		extern DLL_LINKAGE boost::filesystem::path g_server_path;

		extern DLL_LINKAGE std::vector<boost::filesystem::path> g_data_paths;

		extern DLL_LINKAGE std::string g_help_string;
	}

	/// deprecated: get singleton instance
	extern DLL_LINKAGE _VCMIDirs & get();

	// Path to user-specific data directory
	inline const boost::filesystem::path& userDataPath() { return detail::g_user_data_path; }

	// Path to "cache" directory, can be used for any non-essential files
	inline const boost::filesystem::path& userCachePath() { return detail::g_user_cache_path; }

	// Path to writeable directory with user configs
	inline const boost::filesystem::path& userConfigPath() { return detail::g_user_config_path; }

	// Path to saved games
	inline const boost::filesystem::path& userSavePath() { return detail::g_user_save_path; }

	// Paths to global system-wide data directories. First items have higher priority
	inline const std::vector<boost::filesystem::path>& dataPaths() { return detail::g_data_paths; }

	// Full path to client executable, including server name (e.g. /usr/bin/vcmiclient)
	inline const boost::filesystem::path& clientPath() { return detail::g_client_path; }

	// Full path to server executable, including server name (e.g. /usr/bin/vcmiserver)
	inline const boost::filesystem::path& serverPath() { return detail::g_server_path; }

	// Path where vcmi libraries can be found (in AI and Scripting subdirectories)
	inline const boost::filesystem::path& libraryPath() { return detail::g_library_path; }

	// Returns system-specific name for dynamic libraries ( StupidAI => "libStupidAI.so" or "StupidAI.dll")
	extern DLL_LINKAGE std::string libraryName(const std::string& basename);
	//extern DLL_LINKAGE std::string libraryName(const char* basename);
	//extern DLL_LINKAGE std::string libraryName(std::string&& basename);


	inline const std::string& genHelpString() { return detail::g_help_string; }
}

// TODO: Remove _VCMIDirs
// This class is deprecated
class DLL_LINKAGE _VCMIDirs
{
public:
	/// deprecated: Path to user-specific data directory
	std::string userDataPath() const { return VCMIDirs::userDataPath().string(); }

	/// deprecated: Path to "cache" directory, can be used for any non-essential files
	std::string userCachePath() const { return VCMIDirs::userCachePath().string(); }

	/// deprecated: Path to writeable directory with user configs
	std::string userConfigPath() const { return VCMIDirs::userConfigPath().string(); }

	/// deprecated: Path to saved games
	std::string userSavePath() const { return VCMIDirs::userSavePath().string(); }

	/// deprecated: Paths to global system-wide data directories. First items have higher priority
	std::vector<std::string> dataPaths() const
	{
		std::vector<std::string> result;
		for (const auto& path : VCMIDirs::dataPaths())
			result.push_back(path.string());
		return result;
	}

	/// deprecated: Full path to client executable, including server name (e.g. /usr/bin/vcmiclient)
	std::string clientPath() const { return VCMIDirs::clientPath().string(); }

	/// deprecated: Full path to server executable, including server name (e.g. /usr/bin/vcmiserver)
	std::string serverPath() const { return VCMIDirs::serverPath().string(); }

	/// deprecated: Path where vcmi libraries can be found (in AI and Scripting subdirectories)
	std::string libraryPath() const { return VCMIDirs::libraryPath().string(); }

	/// deprecated: Returns system-specific name for dynamic libraries ( StupidAI => "libStupidAI.so" or "StupidAI.dll")
	std::string libraryName(std::string basename) const { return VCMIDirs::libraryName(basename); }

	/// deprecated:
	std::string genHelpString() const { return VCMIDirs::genHelpString(); }
};