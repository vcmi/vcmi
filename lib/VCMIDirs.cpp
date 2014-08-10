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

bfs::path IVCMIDirs::userSavePath() const { return userDataPath() / "Saves"; }

void IVCMIDirs::init()
{
	// TODO: Log errors
	bfs::create_directory(userDataPath());
	bfs::create_directory(userCachePath());
	bfs::create_directory(userConfigPath());
	bfs::create_directory(userSavePath());
}

#ifdef VCMI_WINDOWS

#include <Windows.h>
#include <Shlobj.h>
#include <Shellapi.h>

class VCMIDirs_win32 : public IVCMIDirs
{
	public:
		boost::filesystem::path userDataPath() const override;
		boost::filesystem::path userCachePath() const override;
		boost::filesystem::path userConfigPath() const override;

		std::vector<boost::filesystem::path> dataPaths() const override;

		boost::filesystem::path clientPath() const override;
		boost::filesystem::path serverPath() const override;

		boost::filesystem::path libraryPath() const override;
		boost::filesystem::path binaryPath() const override;

		std::string libraryName(const std::string& basename) const override;

		std::string genHelpString() const override;

		void init() override;
};

void VCMIDirs_win32::init()
{
	// Call base (init dirs)
	IVCMIDirs::init();

	auto moveDirIfExists = [](const bfs::path& from, const bfs::path& to)
	{
		if (!bfs::is_directory(from))
			return; // Nothing to do here. Flies away.

		if (bfs::is_empty(from))
		{
			bfs::remove(from);
			return; // Nothing to do here. Flies away.
		}

		if (!bfs::is_directory(to))
		{
			// IVCMIDirs::init() should create all destination directories.
			// TODO: Log fact, that we shouldn't be here.
			bfs::create_directory(to);
		}

		// Why the hell path strings should be end with double null :/
		auto make_double_nulled = [](const bfs::path& path) -> std::unique_ptr<wchar_t[]>
		{
			const std::wstring& path_str = path.native();
			std::unique_ptr<wchar_t[]> result(new wchar_t[path_str.length() + 2]);
			
			size_t i = 0;
			for (const wchar_t ch : path_str)
				result[i++] = ch;
			result[i++] = L'\0';
			result[i++] = L'\0';

			return result;
		};

		auto from_dnulled = make_double_nulled(from / L"*.*");
		auto to_dnulled = make_double_nulled(to);

		SHFILEOPSTRUCTW file_op;
		file_op.hwnd = GetConsoleWindow();
		file_op.wFunc = FO_MOVE;
		file_op.pFrom = from_dnulled.get();
		file_op.pTo = to_dnulled.get();
		file_op.fFlags = 0;
		file_op.hNameMappings = nullptr;
		file_op.lpszProgressTitle = nullptr;

		const int error_code = SHFileOperationW(&file_op);
		if (error_code != 0); // TODO: Log error. User should try to move files.
		else if (file_op.fAnyOperationsAborted); // TODO: Log warn. User aborted operation. User should move files.
		else if (!bfs::is_empty(from)); // TODO: Log warn. Some files not moved. User should try to move files.
		else // TODO: Log fact that we moved files succefully.
			bfs::remove(from);
	};

	moveDirIfExists(userDataPath() / "Games", userSavePath());
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
		// FIXME: Use _wdupenv_s on MS Visual Studio.
		//    or: define _CRT_SECURE_NO_WARNINGS in preprocessor global settings.
		// warning C4996: 'getenv': This function or variable may be unsafe.
		// Consider using _dupenv_s instead.
		// To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
		// See online help for details.
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

	std::vector<std::string> temp_vec;
	for (const bfs::path& path : dataPaths())
		temp_vec.push_back(path.string());
	std::string gd_string_a = boost::algorithm::join(temp_vec, ";");


	return
		"  game data:   " + gd_string_a + "\n"
		"  libraries:   " + libraryPath().string() + "\n"
		"  server:      " + serverPath().string() + "\n"
		"\n"
		"  user data:   " + userDataPath().string() + "\n"
		"  user cache:  " + userCachePath().string() + "\n"
		"  user config: " + userConfigPath().string() + "\n"
		"  user saves:  " + userSavePath().string() + "\n"; // Should end without new-line?
}

std::string VCMIDirs_win32::libraryName(const std::string& basename) const { return basename + ".dll"; }
#elif defined(VCMI_UNIX)
class IVCMIDirs_UNIX : public IVCMIDirs
{
	public:
		boost::filesystem::path clientPath() const override;
		boost::filesystem::path serverPath() const override;

		std::string genHelpString() const override;
};

bfs::path IVCMIDirs_UNIX::clientPath() const { return binaryPath() / "vcmiclient"; }
bfs::path IVCMIDirs_UNIX::clientPath() const { return binaryPath() / "vcmiserver"; }

std::string IVCMIDirs_UNIX::genHelpString() const
{
	std::vector<std::string> temp_vec;
	for (const bfs::path& path : dataPaths())
		temp_vec.push_back(path.string());
	std::string gd_string_a = boost::algorithm::join(temp_vec, ":");


	return
		"  game data:   " + gd_string_a + "\n"
		"  libraries:   " + libraryPath().string() + "\n"
		"  server:      " + serverPath().string() + "\n"
		"\n"
		"  user data:   " + userDataPath().string() + "\n"
		"  user cache:  " + userCachePath().string() + "\n"
		"  user config: " + userConfigPath().string() + "\n"
		"  user saves:  " + userSavePath().string() + "\n"; // Should end without new-line?
}

#ifdef VCMI_APPLE
class VCMIDirs_OSX : public IVCMIDirs_UNIX
{
	public:
		boost::filesystem::path userDataPath() const override;
		boost::filesystem::path userCachePath() const override;
		boost::filesystem::path userConfigPath() const override;

		std::vector<boost::filesystem::path> dataPaths() const override;

		boost::filesystem::path libraryPath() const override;
		boost::filesystem::path binaryPath() const override;

		std::string libraryName(const std::string& basename) const override;

		void init() override;
};

void VCMIDirs_OSX::init()
{
	// Call base (init dirs)
	IVCMIDirs_UNIX::init();

	auto moveDirIfExists = [](const bfs::path& from, const bfs::path& to)
	{
		if (!bfs::is_directory(from))
			return; // Nothing to do here. Flies away.

		if (bfs::is_empty(from))
		{
			bfs::remove(from);
			return; // Nothing to do here. Flies away.
		}

		if (!bfs::is_directory(to))
		{
			// IVCMIDirs::init() should create all destination directories.
			// TODO: Log fact, that we shouldn't be here.
			bfs::create_directory(to);
		}

		for (bfs::directory_iterator file(from); file != bfs::directory_iterator(); ++file)
		{
			const boost::filesystem::path& src_file_path = file->path();
			const boost::filesystem::path  dst_file_path = to / src_file_path.filename();

			// TODO: Aplication should ask user what to do when file exists:
			// replace/ignore/stop process/replace all/ignore all
			if (!boost::filesystem::exists(dst_file_path))
				bfs::rename(src_file_path, dst_file_path);
		}

		if (!bfs::is_empty(from)); // TODO: Log warn. Some files not moved. User should try to move files.
		else
			bfs::remove(from);
	};

	moveDirIfExists(userDataPath() / "Games", userSavePath());
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

std::vector<bfs::path> VCMIDirs_OSX::dataPaths() const
{
	return std::vector<bfs::path>(1, "../Data");
}

bfs::path VCMIDirs_OSX::libraryPath() const { return "."; }
bfs::path VCMIDirs_OSX::binaryPath() const { return "."; }

std::string libraryName(const std::string& basename) { return "lib" + basename + ".dylib"; }
#elif defined(VCMI_LINUX)
class VCMIDirs_Linux : public IVCMIDirs_UNIX
{
public:
	boost::filesystem::path userDataPath() const override;
	boost::filesystem::path userCachePath() const override;
	boost::filesystem::path userConfigPath() const override;

	std::vector<boost::filesystem::path> dataPaths() const override;

	boost::filesystem::path libraryPath() const override;
	boost::filesystem::path binaryPath() const override;

	std::string libraryName(const std::string& basename) const override;
};

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

std::vector<bfs::path> VCMIDirs_Linux::dataPaths() const
{
	// $XDG_DATA_DIRS, default: /usr/local/share/:/usr/share/

	// construct list in reverse.
	// in specification first directory has highest priority
	// in vcmi fs last directory has highest priority
	std::vector<bfs::path> ret;

	const char* home_dir;
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
#ifdef VCMI_ANDROID
class VCMIDirs_Android : public VCMIDirs_Linux
{
public:
	boost::filesystem::path userDataPath() const override;
	boost::filesystem::path userCachePath() const override;
	boost::filesystem::path userConfigPath() const override;

	std::vector<boost::filesystem::path> dataPaths() const override;
};

// on Android HOME will be set to something like /sdcard/data/Android/is.xyz.vcmi/files/
bfs::path VCMIDirs_Android::userDataPath() const { return getenv("HOME"); }
bfs::path VCMIDirs_Android::userCachePath() const { return userDataPath() / "cache"; }
bfs::path VCMIDirs_Android::userConfigPath() const { return userDataPath() / "config"; }

std::vector<bfs::path> VCMIDirs_Android::dataPaths() const
{
	return std::vector<bfs::path>(1, userDataPath());
}
#endif // VCMI_ANDROID
#endif // VCMI_APPLE, VCMI_LINUX
#endif // VCMI_WINDOWS, VCMI_UNIX

// Getters for interfaces are separated for clarity.
namespace VCMIDirs
{
	const IVCMIDirs& get()
	{
		#ifdef VCMI_WINDOWS
			static VCMIDirs_win32 singleton;
		#elif defined(VCMI_ANDROID)
			static VCMIDirs_Android singleton;
		#elif defined(VCMI_LINUX)
			static VCMIDirs_Linux singleton;
		#elif defined(VCMI_APPLE)
			static VCMIDirs_OSX singleton;
		#endif
		static bool initialized = false;
		if (!initialized)
		{
			singleton.init();
			initialized = true;
		}
		return singleton;
	}
}