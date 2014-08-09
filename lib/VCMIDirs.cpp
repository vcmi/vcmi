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

namespace bfs = boost::filesystem; // Should be in each cpp file

#ifdef _WIN32
// File: VCMIDirs_win32.h
//#include "IVCMIDirs.h"

class VCMIDirs_win32 : public IVCMIDirs
{
	public:
		boost::filesystem::path userDataPath() const override;
		boost::filesystem::path userCachePath() const override;
		boost::filesystem::path userConfigPath() const override;
		boost::filesystem::path userSavePath() const override;

		std::vector<boost::filesystem::path> dataPaths() const override;

		boost::filesystem::path clientPath() const override;
		boost::filesystem::path serverPath() const override;

		boost::filesystem::path libraryPath() const override;
		boost::filesystem::path binaryPath() const override;

		std::string libraryName(const std::string& basename) const override;

		std::string genHelpString() const override;
};
// End of file: VCMIDirs_win32.h

// File: VCMIDirs_win32.cpp
//#include "StdInc.h"
//#include "VCMIDirs_win32"
// WinAPI
#include <Windows.h>	// WideCharToMultiByte
#include <Shlobj.h>		// SHGetSpecialFolderPathW

namespace VCMIDirs
{
	const IVCMIDirs* get()
	{
		static VCMIDirs_win32 singleton;
		return &singleton;
	}
}

bfs::path VCMIDirs_win32::userDataPath() const
{
	const char* profile_dir_a;
	wchar_t profile_dir_w[MAX_PATH];

	// The user's profile folder. A typical path is C:\Users\username.
	// FIXME: Applications should not create files or folders at this level;
	// they should put their data under the locations referred to by CSIDL_APPDATA or CSIDL_LOCAL_APPDATA.
	if (SHGetSpecialFolderPathW(nullptr, profile_dir_w, CSIDL_PROFILE, FALSE) == FALSE) // WinAPI way failed
	{
		// FIXME: Create macro for MS Visual Studio.
		if (profile_dir_a = std::getenv("userprofile")) // STL way succeed
			return bfs::path(profile_dir_a) / "vcmi";
		else
			return "."; // Every thing failed, return current directory.
	}
	else
		return bfs::path(profile_dir_w) / "vcmi";

	//return dataPaths()[0] ???;
}
bfs::path VCMIDirs_win32::userCachePath() const { return userDataPath(); }
bfs::path VCMIDirs_win32::userConfigPath() const { return userDataPath() / "config"; }
bfs::path VCMIDirs_win32::userSavePath() const { return userDataPath() / "Games"; }

std::vector<bfs::path> VCMIDirs_win32::dataPaths() const
{
	return std::vector<bfs::path>(1, bfs::path("."));
}

bfs::path VCMIDirs_win32::clientPath() const { return binaryPath() / "VCMI_client.exe"; }
bfs::path VCMIDirs_win32::serverPath() const { return binaryPath() / "VCMI_server.exe"; }

bfs::path VCMIDirs_win32::libraryPath() const { return "."; }
bfs::path VCMIDirs_win32::binaryPath() const { return ".";  }

std::string VCMIDirs_win32::genHelpString() const
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
	for (const bfs::path& path : dataPaths())
		temp_vec.push_back(utf8_convert(path));
	std::string gd_string_a = boost::algorithm::join(temp_vec, L";");


	return
		"  game data:   " + gd_string_a + "\n" +
		"  libraries:   " + utf8_convert(libraryPath()) + "\n" +
		"  server:      " + utf8_convert(serverPath()) + "\n" +
		"\n" +
		"  user data:   " + utf8_convert(userDataPath()) + "\n" +
		"  user cache:  " + utf8_convert(userCachePath()) + "\n" +
		"  user config: " + utf8_convert(userConfigPath()) + "\n" +
		"  user saves:  " + utf8_convert(userSavePath()) + "\n"; // Should end without new-line?
}

std::string libraryName(const std::string& basename) { return basename + ".dll"; }
// End of file: VCMIDirs_win32.cpp
#else // UNIX
// File: IVCMIDirs_unix.h
//#include "IVCMIDirs.h"

class IVCMIDirs_unix : public IVCMIDirs
{
	public:
		boost::filesystem::path clientPath() const override;
		boost::filesystem::path serverPath() const override;

		std::string genHelpString() const override;
};
// End of file: IVCMIDirs_unix.h

// File: IVCMIDirs_unix.cpp
//#include "StdInc.h"
//#include "IVCMIDirs_unix.h"

bfs::path IVCMIDirs_unix::clientPath() const { return binaryPath() / "vcmiclient"; }
bfs::path IVCMIDirs_unix::clientPath() const { return binaryPath() / "vcmiserver"; }

std::string IVCMIDirs_unix::genHelpString() const
{
	std::vector<std::string> temp_vec;
	for (const bfs::path& path : dataPaths())
		temp_vec.push_back(path.string());
	std::string gd_string_a = boost::algorithm::join(temp_vec, L";");


	return
		"  game data:   " + gd_string_a + "\n" +
		"  libraries:   " + libraryPath().string() + "\n" +
		"  server:      " + serverPath().string() + "\n" +
		"\n" +
		"  user data:   " + userDataPath().string() + "\n" +
		"  user cache:  " + userCachePath().string() + "\n" +
		"  user config: " + userConfigPath().string() + "\n" +
		"  user saves:  " + userSavePath().string() + "\n"; // Should end without new-line?
}
// End of file: IVCMIDirs_unix.cpp

#ifdef __APPLE__
// File: VCMIDirs_OSX.h
//#include "IVCMIDirs_unix.h"

class VCMIDirs_OSX : public IVCMIDirs_unix
{
	public:
		boost::filesystem::path userDataPath() const override;
		boost::filesystem::path userCachePath() const override;
		boost::filesystem::path userConfigPath() const override;
		boost::filesystem::path userSavePath() const override;

		std::vector<boost::filesystem::path> dataPaths() const override;

		boost::filesystem::path libraryPath() const override;
		boost::filesystem::path binaryPath() const override;

		std::string libraryName(const std::string& basename) const override;
};
// End of file: VCMIDirs_OSX.h

// File: VCMIDirs_OSX.cpp
//#include "StdInc.h"
//#include "VCMIDirs_OSX.h"

namespace VCMIDirs
{
	const IVCMIDirs* get()
	{
		static VCMIDirs_OSX singleton;
		return &singleton;
	}
}

bfs::path VCMIDirs_OSX::userDataPath() const
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
bfs::path VCMIDirs_OSX::userCachePath() const { return userDataPath(); }
bfs::path VCMIDirs_OSX::userConfigPath() const { return userDataPath() / "config"; }
bfs::path VCMIDirs_OSX::userSavePath() const { return userDataPath() / "Games"; }

std::vector<bfs::path> VCMIDirs_OSX::dataPaths() const
{
	return std::vector<bfs::path>(1, "../Data");
}

bfs::path VCMIDirs_OSX::libraryPath() const { return "."; }
bfs::path VCMIDirs_OSX::binaryPath() const { return "."; }

std::string libraryName(const std::string& basename) { return "lib" + basename + ".dylib"; }
// End of file: VCMIDirs_OSX.cpp
#else
// File: VCMIDirs_Linux.h
//#include "IVCMIDirs_unix.h"

class VCMIDirs_Linux : public IVCMIDirs_unix
{
public:
	boost::filesystem::path userDataPath() const override;
	boost::filesystem::path userCachePath() const override;
	boost::filesystem::path userConfigPath() const override;
	boost::filesystem::path userSavePath() const override;

	std::vector<boost::filesystem::path> dataPaths() const override;

	boost::filesystem::path libraryPath() const override;
	boost::filesystem::path binaryPath() const override;

	std::string libraryName(const std::string& basename) const override;
};
// End of file: VCMIDirs_Linux.h

// File: VCMIDirs_Linux.cpp
//#include "StdInc.h"
//#include "VCMIDirs_Linux.h"

namespace VCMIDirs
{
	const IVCMIDirs* get()
	{
		static VCMIDirs_Linux singleton;
		return &singleton;
	}
}

bfs::path VCMIDirs_Linux::userDataPath() const
{
	// $XDG_DATA_HOME, default: $HOME/.local/share
	const char* home_dir;
	if (home_dir = getenv("XDG_DATA_HOME"))
		return home_dir;
	else if (home_dir = getenv("HOME"))
		return bfs::path(home_dir) / ".local" / "share" / "vcmi";
	else
		return ".";
}
bfs::path VCMIDirs_Linux::userCachePath() const
{
	// $XDG_CACHE_HOME, default: $HOME/.cache
	const char* home_dir;
	if (home_dir = getenv("XDG_CACHE_HOME"))
		return bfs::path(home_dir) / "vcmi";
	else if (home_dir = getenv("HOME"))
		return bfs::path(home_dir) / ".cache" / "vcmi";
	else
		return ".";
}
bfs::path VCMIDirs_Linux::userConfigPath() const
{
	// $XDG_CONFIG_HOME, default: $HOME/.config
	const char* home_dir;
	if (home_dir = getenv("XDG_CONFIG_HOME"))
		return bfs::path(home_dir) / "vcmi";
	else if (home_dir = getenv("HOME"))
		return bfs::path(home_dir) / ".config" / "vcmi";
	else
		return ".";
}
bfs::path VCMIDirs_Linux::userSavePath() const
{
	return userDataPath() / "Saves";
}

std::vector<bfs::path> VCMIDirs_Linux::dataPaths() const
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

	if ((home_dir = getenv("XDG_DATA_DIRS")) != nullptr)
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

bfs::path VCMIDirs_Linux::libraryPath() const { return M_LIB_PATH; }
bfs::path VCMIDirs_Linux::binaryPath() const { return M_BIN_DIR; }

std::string VCMIDirs_Linux::libraryName(const std::string& basename) const { return "lib" + basename + ".so"; }
// End of file VCMIDirs_Linux.cpp
#ifdef __ANDROID__
// File: VCMIDirs_Android.h
//#include "VCMIDirs_Linux.h"
class VCMIDirs_Android : public VCMIDirs_Linux
{
public:
	boost::filesystem::path userDataPath() const override;
	boost::filesystem::path userCachePath() const override;
	boost::filesystem::path userConfigPath() const override;
	boost::filesystem::path userSavePath() const override;

	std::vector<boost::filesystem::path> dataPaths() const override;
};
// End of file: VCMIDirs_Android.h

// File: VCMIDirs_Android.cpp
//#include "StdInc.h"
//#include "VCMIDirs_Android.h"

namespace VCMIDirs
{
	const IVCMIDirs* get()
	{
		static VCMIDirs_Android singleton;
		return &singleton;
	}
}

// on Android HOME will be set to something like /sdcard/data/Android/is.xyz.vcmi/files/
bfs::path VCMIDirs_Android::userDataPath() const { return getenv("HOME"); }
bfs::path VCMIDirs_Android::userCachePath() const { return userDataPath() / "cache"; }
bfs::path VCMIDirs_Android::userConfigPath() const { return userDataPath() / "config"; }
bfs::path VCMIDirs_Android::userSavePath() const { return userDataPath() / "Saves"; }

std::vector<bfs::path> VCMIDirs_Android::dataPaths() const
{
	return std::vector<bfs::path>(1, userDataPath());
}
// End of file: VCMIDirs_Android.cpp
#endif
#endif
#endif