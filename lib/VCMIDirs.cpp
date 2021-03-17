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

bfs::path IVCMIDirs::userLogsPath() const { return userCachePath(); }

bfs::path IVCMIDirs::userSavePath() const { return userDataPath() / "Saves"; }

bfs::path IVCMIDirs::fullLibraryPath(const std::string &desiredFolder, const std::string &baseLibName) const
{
	return libraryPath() / desiredFolder / libraryName(baseLibName);
}

std::string IVCMIDirs::genHelpString() const
{
	std::vector<std::string> tempVec;
	for (const bfs::path & path : dataPaths())
		tempVec.push_back(path.string());
	const auto gdStringA = boost::algorithm::join(tempVec, ":");

	return
		"  game data:   " + gdStringA + "\n"
		"  libraries:   " + libraryPath().string() + "\n"
		"  server:      " + serverPath().string() + "\n"
		"\n"
		"  user data:   " + userDataPath().string() + "\n"
		"  user cache:  " + userCachePath().string() + "\n"
		"  user config: " + userConfigPath().string() + "\n"
		"  user logs:   " + userLogsPath().string() + "\n"
		"  user saves:  " + userSavePath().string() + "\n"; // Should end without new-line?
}

void IVCMIDirs::init()
{
	// TODO: Log errors
	bfs::create_directories(userDataPath());
	bfs::create_directories(userCachePath());
	bfs::create_directories(userConfigPath());
	bfs::create_directories(userLogsPath());
	bfs::create_directories(userSavePath());
}

#ifdef VCMI_ANDROID
#include "CAndroidVMHelper.h"

#endif

#ifdef VCMI_WINDOWS

#ifdef __MINGW32__
    #define _WIN32_IE 0x0500

	#ifndef CSIDL_MYDOCUMENTS
	#define CSIDL_MYDOCUMENTS CSIDL_PERSONAL
	#endif
#endif // __MINGW32__

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

// Generates script file named _temp.bat in 'to' directory and runs it
// Script will:
// - Wait util 'exeName' ends.
// - Copy all files from 'from' to 'to'
// - Ask user to replace files existed in 'to'.
// - Run 'exeName'
// - Delete itself.
bool StartBatchCopyDataProgram(
	const bfs::path& from, const bfs::path& to, const bfs::path& exeName,
	const bfs::path& currentPath = bfs::current_path())
{
	static const char base[] =
		"@echo off"												"\n"
		"echo Preparing to move VCMI data system."				"\n"

		":CLIENT_RUNNING_LOOP"									"\n"
		"TASKLIST | FIND /I %1% > nul"							"\n"
		"IF ERRORLEVEL 1 ("										"\n"
			"GOTO CLIENT_NOT_RUNNING"							"\n"
		") ELSE ("												"\n"
			"echo %1% is still running..."						"\n"
			"echo Waiting until process ends..."				"\n"
			"ping 1.1.1.1 -n 1 -w 3000 > nul"					"\n" // Sleep ~3 seconds. I love Windows :)
			"goto :CLIENT_RUNNING_LOOP"							"\n"
		")"														"\n"

		":CLIENT_NOT_RUNNING"									"\n"
		"echo %1% turned off..."								"\n"
		"echo Attempt to move datas."							"\n"
		"echo From: %2%"										"\n"
		"echo To: %4%"											"\n"
		"echo Please resolve any conflicts..."					"\n"
		"move /-Y %3% %4%"										"\n" // Move all files from %3% to %4%.
																	 // /-Y ask what to do when file exists in %4%
		":REMOVE_OLD_DIR"										"\n"
		"rd %2% || rem"											"\n" // Remove empty directory. Sets error flag if fail.
		"IF ERRORLEVEL 145 ("									"\n" // Directory not empty
			"echo Directory %2% is not empty."					"\n"
			"echo Please move rest of files manually now."		"\n"
			"pause"												"\n" // Press any key to continue...
			"goto REMOVE_OLD_DIR"								"\n"
		")"														"\n"
		"echo Game data updated succefully."					"\n"
		"echo Please update your shortcuts."					"\n"
		"echo Press any key to start a game . . ."				"\n"
		"pause > nul"											"\n"
		"%5%"													"\n"
		"del \"%%~f0\"&exit"									"\n" // Script deletes itself
		;

	const auto startGameString =
		bfs::equivalent(currentPath, from) ?
		(boost::format("start \"\" %1%") % (to / exeName)) :						// Start game in new path.
		(boost::format("start \"\" /D %1% %2%") % currentPath % (to / exeName));	// Start game in 'currentPath"

	const bfs::path bathFilename = to / "_temp.bat";
	bfs::ofstream bathFile(bathFilename, bfs::ofstream::trunc | bfs::ofstream::out);
	if (!bathFile.is_open())
		return false;
	bathFile << (boost::format(base) % exeName % from % (from / "*.*") % to % startGameString.str()).str();
	bathFile.close();

	std::system(("start \"Updating VCMI datas\" /D \"" + to.string() + "\" \"" + bathFilename.string() + '\"').c_str());
	// start won't block std::system
	// /D start bat in other directory insteand of current directory.

	return true;
}

class VCMIDirsWIN32 final : public IVCMIDirs
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

		void init() override;
	protected:
		boost::filesystem::path oldUserDataPath() const;
		boost::filesystem::path oldUserSavePath() const;
};

void VCMIDirsWIN32::init()
{
	// Call base (init dirs)
	IVCMIDirs::init();

	// Moves one directory (from) contents to another directory (to)
	// Shows user the "moving file dialog" and ask to resolve conflits.
	// If necessary updates current directory.
	auto moveDirIfExists = [](const bfs::path& from, const bfs::path& to) -> bool
	{
		if (!bfs::is_directory(from))
			return true; // Nothing to do here. Flies away.

		if (bfs::is_empty(from))
		{
			if (bfs::current_path() == from)
				bfs::current_path(to);

			bfs::remove(from);
			return true; // Nothing to do here. Flies away.
		}

		if (!bfs::is_directory(to))
		{
			// IVCMIDirs::init() should create all destination directories.
			// TODO: Log fact, that we shouldn't be here.
			bfs::create_directories(to);
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
		if (errorCode != 0) // TODO: Log error. User should try to move files.
			return false;
		else if (fileOp.fAnyOperationsAborted) // TODO: Log warn. User aborted operation. User should move files.
			return false;
		else if (!bfs::is_empty(from)) // TODO: Log warn. Some files not moved. User should try to move files.
			return false;

		if (bfs::current_path() == from)
			bfs::current_path(to);

		// TODO: Log fact that we moved files succefully.
		bfs::remove(from);
		return true;
	};

	// Retrieves the fully qualified path for the file that contains the specified module.
	// The module must have been loaded by the current process.
	// If this parameter is nullptr, retrieves the path of the executable file of the current process.
	auto getModulePath = [](HMODULE hModule) -> bfs::path
	{
		wchar_t exePathW[MAX_PATH];
		DWORD nSize = GetModuleFileNameW(hModule, exePathW, MAX_PATH);
		DWORD error = GetLastError();
		// WARN: Windows XP don't set ERROR_INSUFFICIENT_BUFFER error.
		if (nSize != 0 && error != ERROR_INSUFFICIENT_BUFFER)
			return bfs::path(std::wstring(exePathW, nSize));
		// TODO: Error handling
		return bfs::path();
	};

	// Moves one directory contents to another directory
	// Shows user the "moving file dialog" and ask to resolve conflicts.
	// It takes into account that 'from' path can contain current executable.
	// If necessary closes program and starts update script.
	auto advancedMoveDirIfExists = [getModulePath, moveDirIfExists](const bfs::path& from, const bfs::path& to) -> bool
	{
		const bfs::path executablePath = getModulePath(nullptr);

		// VCMI cann't determine executable path.
		// Use standard way to move directory and exit function.
		if (executablePath.empty())
			return moveDirIfExists(from, to);

		const bfs::path executableName = executablePath.filename();

		// Current executabl isn't in 'from' path.
		// Use standard way to move directory and exit function.
		if (!bfs::equivalent(executablePath, from / executableName))
			return moveDirIfExists(from, to);

		// Try standard way to move directory.
		// I don't know how other systems, but Windows 8.1 allow to move running executable.
		if (moveDirIfExists(from, to))
			return true;

		// Start copying script and exit program.
		if (StartBatchCopyDataProgram(from, to, executableName))
			exit(ERROR_SUCCESS);

		// Everything failed :C
		return false;
	};

	moveDirIfExists(oldUserSavePath(), userSavePath());
	advancedMoveDirIfExists(oldUserDataPath(), userDataPath());
}

bfs::path VCMIDirsWIN32::userDataPath() const
{
	wchar_t profileDir[MAX_PATH];

	if (SHGetSpecialFolderPathW(nullptr, profileDir, CSIDL_MYDOCUMENTS, FALSE) != FALSE)
		return bfs::path(profileDir) / "My Games" / "vcmi";

	return ".";
}

bfs::path VCMIDirsWIN32::oldUserDataPath() const
{
	wchar_t profileDir[MAX_PATH];

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
		if ((profileDirA = std::getenv("userprofile"))) // STL way succeed
			return bfs::path(profileDirA) / "vcmi";
#endif
		else
			return "."; // Every thing failed, return current directory.
	}
	else
		return bfs::path(profileDir) / "vcmi";

	//return dataPaths()[0] ???;
}
bfs::path VCMIDirsWIN32::oldUserSavePath() const { return userDataPath() / "Games"; }

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

std::string VCMIDirsWIN32::libraryName(const std::string& basename) const { return basename + ".dll"; }
#elif defined(VCMI_UNIX)
class IVCMIDirsUNIX : public IVCMIDirs
{
	public:
		boost::filesystem::path clientPath() const override;
		boost::filesystem::path serverPath() const override;

		virtual bool developmentMode() const;
};

bool IVCMIDirsUNIX::developmentMode() const
{
	// We want to be able to run VCMI from single directory. E.g to run from build output directory
	return bfs::exists("AI") && bfs::exists("config") && bfs::exists("Mods") && bfs::exists("vcmiserver") && bfs::exists("vcmiclient");
}

bfs::path IVCMIDirsUNIX::clientPath() const { return binaryPath() / "vcmiclient"; }
bfs::path IVCMIDirsUNIX::serverPath() const { return binaryPath() / "vcmiserver"; }

#ifdef VCMI_APPLE
class VCMIDirsApple : public IVCMIDirsUNIX
{
	public:
		bfs::path userConfigPath() const override;

		std::string libraryName(const std::string& basename) const override;
};

bfs::path VCMIDirsApple::userConfigPath() const { return userDataPath() / "config"; }

std::string VCMIDirsApple::libraryName(const std::string& basename) const { return "lib" + basename + ".dylib"; }

#ifdef VCMI_IOS
extern "C" {
#import "CIOSUtils.h"
}

class VCMIDirsIOS final : public VCMIDirsApple
{
	public:
		bfs::path userDataPath() const override;
		bfs::path userCachePath() const override;
		bfs::path userLogsPath() const override;

		std::vector<bfs::path> dataPaths() const override;

		bfs::path libraryPath() const override;
		bfs::path binaryPath() const override;

		bool developmentMode() const override;
};

bfs::path VCMIDirsIOS::userDataPath() const { return {ios_documentsPath()}; }
bfs::path VCMIDirsIOS::userCachePath() const { return {ios_cachesPath()}; }
bfs::path VCMIDirsIOS::userLogsPath() const { return {ios_documentsPath()}; }

std::vector<bfs::path> VCMIDirsIOS::dataPaths() const
{
    return {
#ifdef VCMI_IOS_SIM
        // fixme ios
        {"/Users/kambala/Library/Application Support/vcmi"},
#endif
        binaryPath(),
        userDataPath(),
    };
}

bfs::path VCMIDirsIOS::libraryPath() const { return {ios_frameworksPath()}; }
bfs::path VCMIDirsIOS::binaryPath() const { return {ios_bundlePath()}; }

bool VCMIDirsIOS::developmentMode() const { return false; }
#elif defined(VCMI_MAC)
class VCMIDirsOSX final : public VCMIDirsApple
{
	public:
		boost::filesystem::path userDataPath() const override;
		boost::filesystem::path userCachePath() const override;
		boost::filesystem::path userLogsPath() const override;

		std::vector<boost::filesystem::path> dataPaths() const override;

		boost::filesystem::path libraryPath() const override;
		boost::filesystem::path binaryPath() const override;

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
			bfs::create_directories(to);
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

bfs::path VCMIDirsOSX::userLogsPath() const
{
	// TODO: use proper objc code from Foundation framework
	if(const auto homeDir = std::getenv("HOME"))
		return bfs::path{homeDir} / "Library" / "Logs" / "vcmi";
	return IVCMIDirsUNIX::userLogsPath();
}

std::vector<bfs::path> VCMIDirsOSX::dataPaths() const
{
	std::vector<bfs::path> ret;
	//FIXME: need some proper codepath for detecting running from build output directory
	if(developmentMode())
	{
		ret.push_back(".");
	}
	else
	{
		ret.push_back("../Resources/Data");
	}
	return ret;
}

bfs::path VCMIDirsOSX::libraryPath() const { return "."; }
bfs::path VCMIDirsOSX::binaryPath() const { return "."; }
#endif // VCMI_IOS, VCMI_MAC
#elif defined(VCMI_XDG)
class VCMIDirsXDG : public IVCMIDirsUNIX
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

bfs::path VCMIDirsXDG::userDataPath() const
{
	// $XDG_DATA_HOME, default: $HOME/.local/share
	const char* homeDir;
	if((homeDir = getenv("XDG_DATA_HOME")))
		return bfs::path(homeDir) / "vcmi";
	else if((homeDir = getenv("HOME")))
		return bfs::path(homeDir) / ".local" / "share" / "vcmi";
	else
		return ".";
}
bfs::path VCMIDirsXDG::userCachePath() const
{
	// $XDG_CACHE_HOME, default: $HOME/.cache
	const char * tempResult;
	if ((tempResult = getenv("XDG_CACHE_HOME")))
		return bfs::path(tempResult) / "vcmi";
	else if ((tempResult = getenv("HOME")))
		return bfs::path(tempResult) / ".cache" / "vcmi";
	else
		return ".";
}
bfs::path VCMIDirsXDG::userConfigPath() const
{
	// $XDG_CONFIG_HOME, default: $HOME/.config
	const char * tempResult;
	if ((tempResult = getenv("XDG_CONFIG_HOME")))
		return bfs::path(tempResult) / "vcmi";
	else if ((tempResult = getenv("HOME")))
		return bfs::path(tempResult) / ".config" / "vcmi";
	else
		return ".";
}

std::vector<bfs::path> VCMIDirsXDG::dataPaths() const
{
	// $XDG_DATA_DIRS, default: /usr/local/share/:/usr/share/

	// construct list in reverse.
	// in specification first directory has highest priority
	// in vcmi fs last directory has highest priority
	std::vector<bfs::path> ret;

	if(developmentMode())
	{
		//For now we'll disable usage of system directories when VCMI running from bin directory
		ret.push_back(".");
	}
	else
	{
		ret.push_back(M_DATA_DIR);
		const char * tempResult;
		if((tempResult = getenv("XDG_DATA_DIRS")) != nullptr)
		{
			std::string dataDirsEnv = tempResult;
			std::vector<std::string> dataDirs;
			boost::split(dataDirs, dataDirsEnv, boost::is_any_of(":"));
			for (auto & entry : boost::adaptors::reverse(dataDirs))
				ret.push_back(bfs::path(entry) / "vcmi");
		}
		else
		{
			ret.push_back(bfs::path("/usr/share") / "vcmi");
			ret.push_back(bfs::path("/usr/local/share") / "vcmi");
		}

		// Debian and other distributions might want to use it while it's not part of XDG
		ret.push_back(bfs::path("/usr/share/games") / "vcmi");
	}

	return ret;
}

bfs::path VCMIDirsXDG::libraryPath() const
{
	if(developmentMode())
		return ".";
	else
		return M_LIB_DIR;
}

bfs::path VCMIDirsXDG::binaryPath() const
{
	if(developmentMode())
		return ".";
	else
		return M_BIN_DIR;
}

std::string VCMIDirsXDG::libraryName(const std::string& basename) const { return "lib" + basename + ".so"; }

#ifdef VCMI_ANDROID

class VCMIDirsAndroid : public VCMIDirsXDG
{
	std::string basePath;
	std::string internalPath;
	std::string nativePath;
public:
	bfs::path fullLibraryPath(const std::string & desiredFolder, const std::string & baseLibName) const override;
	bfs::path libraryPath() const override;
	bfs::path userDataPath() const override;
	bfs::path userCachePath() const override;
	bfs::path userConfigPath() const override;

	std::vector<bfs::path> dataPaths() const override;

	void init() override;
};

bfs::path VCMIDirsAndroid::libraryPath() const { return nativePath; }
bfs::path VCMIDirsAndroid::userDataPath() const { return basePath; }
bfs::path VCMIDirsAndroid::userCachePath() const { return userDataPath() / "cache"; }
bfs::path VCMIDirsAndroid::userConfigPath() const { return userDataPath() / "config"; }

bfs::path VCMIDirsAndroid::fullLibraryPath(const std::string & desiredFolder, const std::string & baseLibName) const
{
	// ignore passed folder (all libraries in android are dumped into a single folder)
	return libraryPath() / libraryName(baseLibName);
}

std::vector<bfs::path> VCMIDirsAndroid::dataPaths() const
{
	std::vector<bfs::path> paths(2);
	paths.push_back(internalPath);
	paths.push_back(userDataPath());
	return paths;
}

void VCMIDirsAndroid::init()
{
	// asks java code to retrieve needed paths from environment
	CAndroidVMHelper envHelper;
	basePath = envHelper.callStaticStringMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "dataRoot");
	internalPath = envHelper.callStaticStringMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "internalDataRoot");
	nativePath = envHelper.callStaticStringMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "nativePath");
	IVCMIDirs::init();
}

#endif // VCMI_ANDROID
#endif // VCMI_APPLE, VCMI_XDG
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
		#elif defined(VCMI_XDG)
			static VCMIDirsXDG singleton;
		#elif defined(VCMI_MAC)
			static VCMIDirsOSX singleton;
		#elif defined(VCMI_IOS)
			static VCMIDirsIOS singleton;
		#endif

		static bool initialized = false;
		if (!initialized)
		{
			#ifdef VCMI_WINDOWS
			std::locale::global(boost::locale::generator().generate("en_US.UTF-8"));
			#endif
			boost::filesystem::path::imbue(std::locale());

			singleton.init();
			initialized = true;
		}
		return singleton;
	}
}

