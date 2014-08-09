/*
 * VCMIDirs.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "VCMIDirs.h"

namespace bfs = boost::filesystem;

namespace VCMIDirs
{
	namespace detail
	{
		bfs::path g_user_data_path;
		bfs::path g_user_cache_path;
		bfs::path g_user_config_path;
		bfs::path g_user_save_path;
		
		bfs::path g_library_path;
		bfs::path g_client_path;
		bfs::path g_server_path;

		std::vector<bfs::path> g_data_paths;

		std::string g_help_string;

		extern bfs::path GetDataPath();
		extern bfs::path GetCachePath();
		extern bfs::path GetConfigPath();
		extern bfs::path GetUserSavePath();
		
		extern bfs::path GetLibraryPath();
		extern bfs::path GetClientPath();
		extern bfs::path GetServerPath();
		extern std::vector<bfs::path> GetDataPaths();
	
		extern std::string GetHelpString();
	}
	
	void InitAllPathes(void)
	{
		detail::g_user_data_path = detail::GetDataPath();
		detail::g_user_cache_path = detail::GetCachePath();
		detail::g_user_config_path = detail::GetConfigPath();
		detail::g_user_save_path = detail::GetUserSavePath();

		detail::g_library_path = detail::GetLibraryPath();
		detail::g_client_path = detail::GetClientPath();
		detail::g_server_path = detail::GetServerPath();

		detail::g_data_paths = detail::GetDataPaths();

		detail::g_help_string = detail::GetHelpString();

		// initialize local directory and create folders to which VCMI needs write access
		bfs::create_directory(detail::g_user_data_path);
		bfs::create_directory(detail::g_user_cache_path);
		bfs::create_directory(detail::g_user_config_path);
		bfs::create_directory(detail::g_user_save_path);
	}

	namespace detail
	{
		struct InitAllPathes_ctor
		{
			InitAllPathes_ctor() { InitAllPathes(); }
		} InitAllPathes_ctor_global_obj;
	};

	// TODO: Remove _VCMIDirs
	static _VCMIDirs _VCMIDirsGlobal;
	_VCMIDirs & get()
	{
		puts("VCMIDirs::get() - used of deprecated function :#");
		return _VCMIDirsGlobal;
	}
}

// FIXME: find way to at least decrease size of this ifdef (along with cleanup in CMake)
#ifdef _WIN32
// This part should be moved into separate file (for example: VCMIDirs_win32.cpp)
// WinAPI
#include <Windows.h>
#include <Shlobj.h>

namespace VCMIDirs
{
	namespace detail
	{
		bfs::path GetDataPath()
		{
			const char* profile_dir_a;
			wchar_t profile_dir_w[MAX_PATH];

			// The user's profile folder. A typical path is C:\Users\username.
			// FIXME: Applications should not create files or folders at this level;
			// they should put their data under the locations referred to by CSIDL_APPDATA or CSIDL_LOCAL_APPDATA.
			if (SHGetSpecialFolderPathW(nullptr, profile_dir_w, CSIDL_PROFILE, FALSE) == FALSE) // WinAPI way failed
			{
				if (profile_dir_a = std::getenv("userprofile")) // STL way succeed
					return bfs::path(profile_dir_a) / "vcmi";
				else
					return "."; // Every thing failed, return current directory.
			}
			else
				return bfs::path(profile_dir_w) / "vcmi";

			//return dataPaths()[0] ???;
		}

		bfs::path GetCachePath()
		{
			return GetDataPath();
		}

		bfs::path GetConfigPath()
		{
			return GetDataPath() / "config";
		}

		bfs::path GetUserSavePath()
		{
			return GetDataPath() / "Games";
		}

		bfs::path GetLibraryPath()
		{
			return ".";
		}

		bfs::path GetClientPath()
		{
			return GetLibraryPath() / "VCMI_client.exe";
		}

		bfs::path GetServerPath()
		{
			return GetLibraryPath() / "VCMI_server.exe";
		}
	
		std::vector<bfs::path> GetDataPaths()
		{
			return std::vector<bfs::path>(1, bfs::path("."));
		}

		std::string GetHelpString()
		{
			// I think this function should have multiple versions
			// 1. For various arguments
			// 2. Inverse functions
			// and should be moved to vstd
			// or use http://utfcpp.sourceforge.net/
			auto utf8_convert = [](const bfs::path& path) -> std::string
			{
				const auto& path_string = path.native();
				auto perform_convert = [&path_string](LPSTR output, int output_size)
				{
					return WideCharToMultiByte(
						CP_UTF8,				// Use wchar_t -> utf8 char_t
						WC_ERR_INVALID_CHARS,	// Fails when invalid char occur
						path_string.c_str(),	// String to convert
						path_string.size(),		// String to convert size
						output,					// Result
						output_size,			// Result size
						nullptr, nullptr);		// For the ... CP_UTF8 settings for CodePage, this parameter must be set to NULL
				};

				int char_count = perform_convert(nullptr, 0); // Result size (0 - obtain size)
				if (char_count > 0)
				{
					std::unique_ptr<char[]> buffer(new char[char_count]);
					if ((char_count = perform_convert(buffer.get(), char_count)) > 0)
						return std::string(buffer.get(), char_count);
				}

				// Conversion failed :C
				return path.string();
			};

			std::vector<std::string> temp_vec;
			for (const bfs::path& path : GetDataPaths())
				temp_vec.push_back(utf8_convert(path));
			std::string gd_string_a = boost::algorithm::join(temp_vec, L";");

			
			return
				"  game data:   " + gd_string_a + "\n" +
				"  libraries:   " + utf8_convert(GetLibraryPath()) + "\n" +
				"  server:      " + utf8_convert(GetServerPath()) + "\n" +
				"\n" +
				"  user data:   " + utf8_convert(GetDataPath()) + "\n" +
				"  user cache:  " + utf8_convert(GetCachePath()) + "\n" +
				"  user config: " + utf8_convert(GetConfigPath()) + "\n" +
				"  user saves:  " + utf8_convert(GetUserSavePath()) + "\n"; // Should end without new-line?
		}
	}

	std::string libraryName(const std::string& basename)
	{
		return basename + ".dll";
	}
}

#elif defined (__APPLE__)
// This part should be moved to separate file (for example: VCMIDirs_apple.cpp)

namespace VCMIDirs
{
	namespace detail
	{
		bfs::path GetDataPath()
		{
			// This is Cocoa code that should be normally used to get path to Application Support folder but can't use it here for now...
			// NSArray* urls = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask];
			// UserPath = path([urls[0] path] + "/vcmi").string();

			// ...so here goes a bit of hardcode instead

			const char* home_dir = getenv("HOME"); // Should be std::getenv?
			if (home_dir == nullptr)
				home_dir = ".";
			return bfs::path(home_dir) / "Library" / "Application Support" / "vcmi";
		}

		bfs::path GetCachePath()
		{
			return GetDataPath();
		}

		bfs::path GetConfigPath()
		{
			return GetDataPath() / "config";
		}

		bfs::path GetUserSavePath()
		{
			return GetDataPath() / "Games";
		}

		bfs::path GetLibraryPath()
		{
			return ".";
		}

		bfs::path GetClientPath()
		{
			return "./vcmiclient";
		}

		bfs::path GetServerPath()
		{
			return "./vcmiserver";
		}

		std::vector<bfs::path> GetDataPaths()
		{
			return std::vector<std::string>(1, "../Data");
		}

		std::string GetHelpString()
		{
			std::vector<std::string> temp_vec;
			for (const bfs::path& path : GetDataPaths())
				temp_vec.push_back(path.string());

			return
				"  game data:   " + boost::algorithm::join(temp_vec, ":") + "\n" +
				"  libraries:   " + GetLibraryPath().string() + "\n" +
				"  server:      " + GetServerPath().string() + "\n" +
				"\n" +
				"  user data:   " + GetDataPath().string() + "\n" +
				"  user cache:  " + GetCachePath().string() + "\n" +
				"  user config: " + GetConfigPath().string() + "\n" +
				"  user saves:  " + GetUserSavePath().string() + "\n"; // Should end without new-line?
		}
	}

	std::string libraryName(const std::string& basename)
	{
		return "lib" + basename + ".dylib";
	}
}
#elif defined __ANDROID__
// This part should be moved to separate file (for example: VCMIDirs_android.cpp)

namespace VCMIDirs
{
	namespace detail
	{
		bfs::path GetDataPath()
		{
			// on Android HOME will be set to something like /sdcard/data/Android/is.xyz.vcmi/files/
			return getenv("HOME");
		}

		bfs::path GetCachePath()
		{
			return GetDataPath() / "cache";
		}

		bfs::path GetConfigPath()
		{
			return GetDataPath() / "config";
		}

		bfs::path GetUserSavePath()
		{
			return GetDataPath() / "Saves"; // Why different than other platforms???
		}

		bfs::path GetLibraryPath()
		{
			return M_LIB_DIR;
		}

		bfs::path GetClientPath()
		{
			return bfs::path(M_BIN_DIR) / "vcmiclient";
		}

		bfs::path GetServerPath()
		{
			return bfs::path(M_BIN_DIR) / "vcmiserver";
		}

		std::vector<bfs::path> GetDataPaths()
		{
			return std::vector<bfs::path>(1, GetDataPath());
		}

		std::string GetHelpString()
		{
			std::vector<std::string> temp_vec;
			for (const bfs::path& path : GetDataPaths())
				temp_vec.push_back(path.string());

			return
				"  game data:   " + boost::algorithm::join(temp_vec, ":") + "\n" +
				"  libraries:   " + GetLibraryPath().string() + "\n" +
				"  server:      " + GetServerPath().string() + "\n" +
				"\n" +
				"  user data:   " + GetDataPath().string() + "\n" +
				"  user cache:  " + GetCachePath().string() + "\n" +
				"  user config: " + GetConfigPath().string() + "\n" +
				"  user saves:  " + GetUserSavePath().string() + "\n"; // Should end without new-line?
		}
	}

	std::string libraryName(const std::string& basename)
	{
		return "lib" + basename + ".so";
	}
}
#else
// This part should be moved to separate file (for example: VCMIDirs_default.cpp)

namespace VCMIDirs
{
	namespace detail
	{
		// $XDG_DATA_HOME, default: $HOME/.local/share
		bfs::path GetDataPath()
		{
			const char* home_dir;
			if (home_dir = getenv("XDG_DATA_HOME"))
				return = home_dir;
			else if (home_dir = getenv("HOME"))
				return = bfs::path(home_dir) / ".local" / "share" / "vcmi";
			else
				return ".";
		}
		
		// $XDG_CACHE_HOME, default: $HOME/.cache
		bfs::path GetCachePath()
		{
			const char* home_dir;
			if (home_dir = getenv("XDG_CACHE_HOME"))
				return bfs::path(home_dir) / "vcmi";
			else if (home_dir = getenv("HOME"))
				return bfs::path(home_dir) / ".cache" / "vcmi";
			else
				return ".";
		}

		// $XDG_CONFIG_HOME, default: $HOME/.config
		bfs::path GetConfigPath()
		{
			const char* home_dir;
			if (home_dir = getenv("XDG_CONFIG_HOME"))
				return bfs:path(home_dir) / "vcmi";
			else if (home_dir = getenv("HOME"))
				return bfs::path(home_dir) / ".config" / "vcmi";
			else
				return ".";
		}

		bfs::path GetUserSavePath()
		{
			return GetDataPath() / "Saves"; // Why different than other platforms???
		}

		bfs::path GetLibraryPath()
		{
			return M_LIB_DIR;
		}

		bfs::path GetClientPath()
		{
			return bfs::path(M_BIN_DIR) / "vcmiclient";
		}

		bfs::path GetServerPath()
		{
			return bfs::path(M_BIN_DIR) / "vcmiserver";
		}

		std::vector<bfs::path> GetDataPaths()
		{
			// $XDG_DATA_DIRS, default: /usr/local/share/:/usr/share/

			// construct list in reverse.
			// in specification first directory has highest priority
			// in vcmi fs last directory has highest priority
			std::vector<bfs::path> ret;

			const char* home_dir;
			if (home_dir = getenv("HOME")) // compatibility, should be removed after 0.96
				ret.push_back(bfs::path(home_dir) / ".vcmi");
			ret.push_back(M_DATA_DIR);

			if (home_dir = getenv("XDG_DATA_DIRS") != nullptr)
			{
				std::string dataDirsEnv = home_dir;
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

		std::string GetHelpString()
		{
			std::vector<std::string> temp_vec;
			for (const bfs::path& path : GetDataPaths())
				temp_vec.push_back(path.string());

			return
				"  game data:   " + boost::algorithm::join(temp_vec, ":") + "\n" +
				"  libraries:   " + GetLibraryPath().string() + "\n" +
				"  server:      " + GetServerPath().string() + "\n" +
				"\n" +
				"  user data:   " + GetDataPath().string() + "\n" +
				"  user cache:  " + GetCachePath().string() + "\n" +
				"  user config: " + GetConfigPath().string() + "\n" +
				"  user saves:  " + GetUserSavePath().string() + "\n"; // Should end without new-line?
		}
	}

	std::string libraryName(const std::string& basename)
	{
		return "lib" + basename + ".so";
	}
}
#endif