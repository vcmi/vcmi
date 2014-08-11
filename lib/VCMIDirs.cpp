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

class VCMIDirsWIN32 : public IVCMIDirs
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

void VCMIDirsWIN32::init()
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
		auto makeDoubleNulled = [](const bfs::path& path) -> std::unique_ptr<wchar_t[]>
		{
			const std::wstring& pathStr = path.native();
			std::unique_ptr<wchar_t[]> result(new wchar_t[pathStr.length() + 2]);
			
			size_t i = 0;
			for (const wchar_t ch : pathStr)
				result[i++] = ch;
			result[i++] = L'\0';
			result[i++] = L'\0';

			return result;
		};

		auto fromDNulled = makeDoubleNulled(from / L"*.*");
		auto toDNulled = makeDoubleNulled(to);

		SHFILEOPSTRUCTW fileOp;
		fileOp.hwnd = GetConsoleWindow();
		fileOp.wFunc = FO_MOVE;
		fileOp.pFrom = fromDNulled.get();
		fileOp.pTo = toDNulled.get();
		fileOp.fFlags = 0;
		fileOp.hNameMappings = nullptr;
		fileOp.lpszProgressTitle = nullptr;

		const int errorCode = SHFileOperationW(&fileOp);
		if (errorCode != 0); // TODO: Log error. User should try to move files.
		else if (fileOp.fAnyOperationsAborted); // TODO: Log warn. User aborted operation. User should move files.
		else if (!bfs::is_empty(from)); // TODO: Log warn. Some files not moved. User should try to move files.
		else // TODO: Log fact that we moved files succefully.
			bfs::remove(from);
	};

	moveDirIfExists(userDataPath() / "Games", userSavePath());
}

bfs::path VCMIDirsWIN32::userDataPath() const
{
	wchar_t profileDir[MAX_PATH];

	// The user's profile folder. A typical path is C:\Users\username.
	// FIXME: Applications should not create files or folders at this level;
	// they should put their data under the locations referred to by CSIDL_APPDATA or CSIDL_LOCAL_APPDATA.
	if (SHGetSpecialFolderPathW(nullptr, profileDir, CSIDL_PROFILE, FALSE) == FALSE) // WinAPI way failed
	{
#if defined(_MSC_VER) && _MSC_VER >= 1700
		wchar_t* buffer;
		size_t bufferSize;
		errno_t result = _wdupenv_s(&buffer, &bufferSize, L"userprofile");
		if (result == 0)
		{
			bfs::path result(std::wstring(buffer, bufferSize));
			free(buffer);
			return result;
		}
#else
		const char* profileDirA;
		if (profileDirA = std::getenv("userprofile")) // STL way succeed
			return bfs::path(profileDirA) / "vcmi";
#endif
		else
			return "."; // Every thing failed, return current directory.
	}
	else
		return bfs::path(profileDir) / "vcmi";

	//return dataPaths()[0] ???;
}
bfs::path VCMIDirsWIN32::userCachePath() const { return userDataPath(); }
bfs::path VCMIDirsWIN32::userConfigPath() const { return userDataPath() / "config"; }

std::vector<bfs::path> VCMIDirsWIN32::dataPaths() const
{
	return std::vector<bfs::path>(1, bfs::path("."));
}

bfs::path VCMIDirsWIN32::clientPath() const { return binaryPath() / "VCMI_client.exe"; }
bfs::path VCMIDirsWIN32::serverPath() const { return binaryPath() / "VCMI_server.exe"; }

bfs::path VCMIDirsWIN32::libraryPath() const { return "."; }
bfs::path VCMIDirsWIN32::binaryPath() const { return ".";  }

std::string VCMIDirsWIN32::genHelpString() const
{

	std::vector<std::string> tempVec;
	for (const bfs::path& path : dataPaths())
		tempVec.push_back(path.string());
	std::string gdStringA = boost::algorithm::join(tempVec, ";");


	return
		"  game data:   " + gdStringA + "\n"
		"  libraries:   " + libraryPath().string() + "\n"
		"  server:      " + serverPath().string() + "\n"
		"\n"
		"  user data:   " + userDataPath().string() + "\n"
		"  user cache:  " + userCachePath().string() + "\n"
		"  user config: " + userConfigPath().string() + "\n"
		"  user saves:  " + userSavePath().string() + "\n"; // Should end without new-line?
}

std::string VCMIDirsWIN32::libraryName(const std::string& basename) const { return basename + ".dll"; }
#elif defined(VCMI_UNIX)
class IVCMIDirsUNIX : public IVCMIDirs
{
	public:
		boost::filesystem::path clientPath() const override;
		boost::filesystem::path serverPath() const override;

		std::string genHelpString() const override;
};

bfs::path IVCMIDirsUNIX::clientPath() const { return binaryPath() / "vcmiclient"; }
bfs::path IVCMIDirsUNIX::clientPath() const { return binaryPath() / "vcmiserver"; }

std::string IVCMIDirsUNIX::genHelpString() const
{
	std::vector<std::string> tempVec;
	for (const bfs::path& path : dataPaths())
		tempVec.push_back(path.string());
	std::string gdStringA = boost::algorithm::join(tempVec, ":");


	return
		"  game data:   " + gdStringA + "\n"
		"  libraries:   " + libraryPath().string() + "\n"
		"  server:      " + serverPath().string() + "\n"
		"\n"
		"  user data:   " + userDataPath().string() + "\n"
		"  user cache:  " + userCachePath().string() + "\n"
		"  user config: " + userConfigPath().string() + "\n"
		"  user saves:  " + userSavePath().string() + "\n"; // Should end without new-line?
}

#ifdef VCMI_APPLE
class VCMIDirsOSX : public IVCMIDirsUNIX
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

void VCMIDirsOSX::init()
{
	// Call base (init dirs)
	IVCMIDirsUNIX::init();

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
			const boost::filesystem::path& srcFilePath = file->path();
			const boost::filesystem::path  dstFilePath = to / srcFilePath.filename();

			// TODO: Aplication should ask user what to do when file exists:
			// replace/ignore/stop process/replace all/ignore all
			if (!boost::filesystem::exists(dstFilePath))
				bfs::rename(srcFilePath, dstFilePath);
		}

		if (!bfs::is_empty(from)); // TODO: Log warn. Some files not moved. User should try to move files.
		else
			bfs::remove(from);
	};

	moveDirIfExists(userDataPath() / "Games", userSavePath());
}

bfs::path VCMIDirsOSX::userDataPath() const
{
	// This is Cocoa code that should be normally used to get path to Application Support folder but can't use it here for now...
	// NSArray* urls = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask];
	// UserPath = path([urls[0] path] + "/vcmi").string();

	// ...so here goes a bit of hardcode instead

	const char* homeDir = getenv("HOME"); // Should be std::getenv?
	if (homeDir == nullptr)
		homeDir = ".";
	return bfs::path(homeDir) / "Library" / "Application Support" / "vcmi";
}
bfs::path VCMIDirsOSX::userCachePath() const { return userDataPath(); }
bfs::path VCMIDirsOSX::userConfigPath() const { return userDataPath() / "config"; }

std::vector<bfs::path> VCMIDirsOSX::dataPaths() const
{
	return std::vector<bfs::path>(1, "../Data");
}

bfs::path VCMIDirsOSX::libraryPath() const { return "."; }
bfs::path VCMIDirsOSX::binaryPath() const { return "."; }

std::string libraryName(const std::string& basename) { return "lib" + basename + ".dylib"; }
#elif defined(VCMI_LINUX)
class VCMIDirsLinux : public IVCMIDirsUNIX
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

bfs::path VCMIDirsLinux::userDataPath() const
{
	// $XDG_DATA_HOME, default: $HOME/.local/share
	const char* homeDir;
	if (homeDir = getenv("XDG_DATA_HOME"))
		return homeDir;
	else if (homeDir = getenv("HOME"))
		return bfs::path(homeDir) / ".local" / "share" / "vcmi";
	else
		return ".";
}
bfs::path VCMIDirsLinux::userCachePath() const
{
	// $XDG_CACHE_HOME, default: $HOME/.cache
	const char* tempResult;
	if (tempResult = getenv("XDG_CACHE_HOME"))
		return bfs::path(tempResult) / "vcmi";
	else if (tempResult = getenv("HOME"))
		return bfs::path(tempResult) / ".cache" / "vcmi";
	else
		return ".";
}
bfs::path VCMIDirsLinux::userConfigPath() const
{
	// $XDG_CONFIG_HOME, default: $HOME/.config
	const char* tempResult;
	if (tempResult = getenv("XDG_CONFIG_HOME"))
		return bfs::path(tempResult) / "vcmi";
	else if (tempResult = getenv("HOME"))
		return bfs::path(tempResult) / ".config" / "vcmi";
	else
		return ".";
}

std::vector<bfs::path> VCMIDirsLinux::dataPaths() const
{
	// $XDG_DATA_DIRS, default: /usr/local/share/:/usr/share/

	// construct list in reverse.
	// in specification first directory has highest priority
	// in vcmi fs last directory has highest priority
	std::vector<bfs::path> ret;

	const char* tempResult;
	ret.push_back(M_DATA_DIR);

	if ((tempResult = getenv("XDG_DATA_DIRS")) != nullptr)
	{
		std::string dataDirsEnv = tempResult;
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

bfs::path VCMIDirsLinux::libraryPath() const { return M_LIB_PATH; }
bfs::path VCMIDirsLinux::binaryPath() const { return M_BIN_DIR; }

std::string VCMIDirsLinux::libraryName(const std::string& basename) const { return "lib" + basename + ".so"; }
#ifdef VCMI_ANDROID
class VCMIDirsAndroid : public VCMIDirsLinux
{
public:
	boost::filesystem::path userDataPath() const override;
	boost::filesystem::path userCachePath() const override;
	boost::filesystem::path userConfigPath() const override;

	std::vector<boost::filesystem::path> dataPaths() const override;
};

// on Android HOME will be set to something like /sdcard/data/Android/is.xyz.vcmi/files/
bfs::path VCMIDirsAndroid::userDataPath() const { return getenv("HOME"); }
bfs::path VCMIDirsAndroid::userCachePath() const { return userDataPath() / "cache"; }
bfs::path VCMIDirsAndroid::userConfigPath() const { return userDataPath() / "config"; }

std::vector<bfs::path> VCMIDirsAndroid::dataPaths() const
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
			static VCMIDirsWIN32 singleton;
		#elif defined(VCMI_ANDROID)
			static VCMIDirsAndroid singleton;
		#elif defined(VCMI_LINUX)
			static VCMIDirsLinux singleton;
		#elif defined(VCMI_APPLE)
			static VCMIDirsOSX singleton;
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