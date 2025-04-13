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
#include "json/JsonNode.h"

#ifdef VCMI_IOS
#include "iOS_utils.h"
#elif defined(VCMI_ANDROID)
#include "CAndroidVMHelper.h"
#endif

VCMI_LIB_NAMESPACE_BEGIN

namespace bfs = boost::filesystem;

bfs::path IVCMIDirs::userLogsPath() const { return userCachePath(); }

bfs::path IVCMIDirs::userSavePath() const { return userDataPath() / "Saves"; }

bfs::path IVCMIDirs::userExtractedPath() const { return userCachePath() / "extracted"; }

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
		"  game data:		" + gdStringA + "\n"
		"  libraries:		" + libraryPath().string() + "\n"
		"  server:			" + serverPath().string() + "\n"
		"\n"
		"  user data:		" + userDataPath().string() + "\n"
		"  user cache:		" + userCachePath().string() + "\n"
		"  user config:		" + userConfigPath().string() + "\n"
		"  user logs:		" + userLogsPath().string() + "\n"
		"  user saves:		" + userSavePath().string() + "\n"
		"  user extracted:	" + userExtractedPath().string() + "\n";
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

#ifdef VCMI_WINDOWS

#ifdef __MINGW32__
    #define _WIN32_IE 0x0500

	#ifndef CSIDL_MYDOCUMENTS
	#define CSIDL_MYDOCUMENTS CSIDL_PERSONAL
	#endif
#endif // __MINGW32__

#include <windows.h>
#include <shlobj.h>

class VCMIDirsWIN32 final : public IVCMIDirs
{
	public:
		VCMIDirsWIN32();
		bfs::path userDataPath() const override;
		bfs::path userCachePath() const override;
		bfs::path userConfigPath() const override;
		bfs::path userLogsPath() const override;
		bfs::path userSavePath() const override;

		std::vector<bfs::path> dataPaths() const override;

		bfs::path clientPath() const override;
		bfs::path mapEditorPath() const override;
		bfs::path serverPath() const override;

		bfs::path libraryPath() const override;
		bfs::path binaryPath() const override;

		std::string libraryName(const std::string& basename) const override;

	protected:
		std::unique_ptr<JsonNode> dirsConfig;

		std::wstring GetRawMyDocumentsPath() const;
		bfs::path getPathFromConfigOrDefault(const std::string& key, const std::function<bfs::path()>& fallbackFunc) const;
		bfs::path getDefaultUserDataPath() const;

		std::wstring utf8ToWstring(const std::string& str) const;
		std::string pathToUtf8(const bfs::path& path) const;
};


VCMIDirsWIN32::VCMIDirsWIN32()
{
	wchar_t currentPath[MAX_PATH];
	GetModuleFileNameW(nullptr, currentPath, MAX_PATH);
	auto configPath = bfs::path(currentPath).parent_path() / "config" / "dirs.json";

	if (!bfs::exists(configPath))
		return;

	std::ifstream in(pathToUtf8(configPath), std::ios::binary);
	if (!in)
		return;

	std::string buffer((std::istreambuf_iterator<char>(in)), {});
	dirsConfig = std::make_unique<JsonNode>(reinterpret_cast<const std::byte*>(buffer.data()), buffer.size(), pathToUtf8(configPath));
}

std::string VCMIDirsWIN32::pathToUtf8(const bfs::path& path) const
{
	std::wstring wstr = path.wstring();
	int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string result(size - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
	return result;
}

std::wstring VCMIDirsWIN32::utf8ToWstring(const std::string& str) const
{
	std::wstring result;
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	if (size_needed > 0)
	{
		result.resize(size_needed - 1);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size_needed);
	}
	return result;
}

std::wstring VCMIDirsWIN32::GetRawMyDocumentsPath() const{
	wchar_t path[MAX_PATH];
	if (SHGetFolderPathW(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, path) == S_OK) {
		return path;
	}
	return L""; 
}

bfs::path VCMIDirsWIN32::getPathFromConfigOrDefault(const std::string& key, const std::function<bfs::path()>& fallbackFunc) const
{
	if (!dirsConfig || !dirsConfig->isStruct())
		return fallbackFunc();

	const JsonNode& node = (*dirsConfig)[key];
	if (!node.isString())
		return fallbackFunc();

	std::wstring raw = utf8ToWstring(node.String());

	const std::wstring placeholder = L"%MyDocuments%";
	size_t pos = raw.find(placeholder);
	if (pos != std::wstring::npos) {
		std::wstring docPath = GetRawMyDocumentsPath();
		if (!docPath.empty()) {
			raw.replace(pos, placeholder.length(), docPath);
		}
	}

	wchar_t expanded[MAX_PATH];
	if (ExpandEnvironmentStringsW(raw.c_str(), expanded, MAX_PATH))
		return bfs::path(expanded);
	else
		return bfs::path(raw);
}

bfs::path VCMIDirsWIN32::getDefaultUserDataPath() const
{
	std::wstring profileDir = GetRawMyDocumentsPath();
	return profileDir.empty() ? bfs::path(".") : bfs::path(profileDir) / "My Games" / "vcmi";
}

bfs::path VCMIDirsWIN32::userDataPath() const
{
	return getPathFromConfigOrDefault("userDataPath", [this] { return getDefaultUserDataPath(); });
}

bfs::path VCMIDirsWIN32::userCachePath() const
{
	return getPathFromConfigOrDefault("userCachePath", [this] { return userDataPath() / "cache"; });
}

bfs::path VCMIDirsWIN32::userConfigPath() const
{
	return getPathFromConfigOrDefault("userConfigPath", [this] { return userDataPath() / "config"; });
}

bfs::path VCMIDirsWIN32::userLogsPath() const
{
	return getPathFromConfigOrDefault("userLogsPath", [this] { return userDataPath() / "logs"; });
}

bfs::path VCMIDirsWIN32::userSavePath() const
{
	return getPathFromConfigOrDefault("userSavePath", [this] { return userDataPath() / "Saves"; });
}

std::vector<bfs::path> VCMIDirsWIN32::dataPaths() const
{
	return std::vector<bfs::path>(1, bfs::path("."));
}

bfs::path VCMIDirsWIN32::clientPath() const { return binaryPath() / "VCMI_client.exe"; }
bfs::path VCMIDirsWIN32::mapEditorPath() const { return binaryPath() / "VCMI_mapeditor.exe"; }
bfs::path VCMIDirsWIN32::serverPath() const { return binaryPath() / "VCMI_server.exe"; }

bfs::path VCMIDirsWIN32::libraryPath() const { return "."; }
bfs::path VCMIDirsWIN32::binaryPath() const { return ".";  }

std::string VCMIDirsWIN32::libraryName(const std::string& basename) const { return basename + ".dll"; }
#elif defined(VCMI_UNIX)
class IVCMIDirsUNIX : public IVCMIDirs
{
	public:
		bfs::path clientPath() const override;
		bfs::path mapEditorPath() const override;
		bfs::path serverPath() const override;

		virtual bool developmentMode() const;
};

bool IVCMIDirsUNIX::developmentMode() const
{
	// We want to be able to run VCMI from single directory. E.g to run from build output directory
	const bool hasConfigs = bfs::exists("config") && bfs::exists("Mods");
	const bool hasBinaries = bfs::exists("vcmiclient") || bfs::exists("vcmiserver") || bfs::exists("vcmilobby");
	return hasConfigs && hasBinaries;
}

bfs::path IVCMIDirsUNIX::clientPath() const { return binaryPath() / "vcmiclient"; }
bfs::path IVCMIDirsUNIX::mapEditorPath() const { return binaryPath() / "vcmieditor"; }
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
class VCMIDirsIOS final : public VCMIDirsApple
{
public:
	bfs::path userDataPath() const override;
	bfs::path userCachePath() const override;
	bfs::path userLogsPath() const override;

	std::vector<bfs::path> dataPaths() const override;

	bfs::path libraryPath() const override;
	bfs::path fullLibraryPath(const std::string & desiredFolder, const std::string & baseLibName) const override;
	bfs::path binaryPath() const override;
};

bfs::path VCMIDirsIOS::userDataPath() const { return {iOS_utils::documentsPath()}; }
bfs::path VCMIDirsIOS::userCachePath() const { return {iOS_utils::cachesPath()}; }
bfs::path VCMIDirsIOS::userLogsPath() const { return {iOS_utils::documentsPath()}; }

std::vector<bfs::path> VCMIDirsIOS::dataPaths() const
{
	std::vector<bfs::path> paths;
	paths.reserve(4);
#ifdef VCMI_IOS_SIM
	paths.emplace_back(iOS_utils::hostApplicationSupportPath());
#endif
	paths.emplace_back(userDataPath());
	paths.emplace_back(iOS_utils::documentsPath());
	paths.emplace_back(binaryPath());
	return paths;
}

bfs::path VCMIDirsIOS::fullLibraryPath(const std::string & desiredFolder, const std::string & baseLibName) const
{
	// iOS has flat libs directory structure
	return libraryPath() / libraryName(baseLibName);
}

bfs::path VCMIDirsIOS::libraryPath() const { return {iOS_utils::frameworksPath()}; }
bfs::path VCMIDirsIOS::binaryPath() const { return {iOS_utils::bundlePath()}; }
#elif defined(VCMI_MAC)
class VCMIDirsOSX final : public VCMIDirsApple
{
public:
	bfs::path userDataPath() const override;
	bfs::path userCachePath() const override;
	bfs::path userLogsPath() const override;

	std::vector<bfs::path> dataPaths() const override;

	bfs::path libraryPath() const override;
	bfs::path binaryPath() const override;

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
			const bfs::path& srcFilePath = file->path();
			const bfs::path  dstFilePath = to / srcFilePath.filename();

			// TODO: Application should ask user what to do when file exists:
			// replace/ignore/stop process/replace all/ignore all
			if (!bfs::exists(dstFilePath))
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

#elif defined(VCMI_ANDROID)
class VCMIDirsAndroid : public IVCMIDirsUNIX
{
	std::string basePath;
	std::string internalPath;
	std::string nativePath;
public:
	std::string libraryName(const std::string & basename) const override;
	bfs::path fullLibraryPath(const std::string & desiredFolder, const std::string & baseLibName) const override;
	bfs::path binaryPath() const override;
	bfs::path libraryPath() const override;
	bfs::path userDataPath() const override;
	bfs::path userCachePath() const override;
	bfs::path userConfigPath() const override;

	std::vector<bfs::path> dataPaths() const override;

	void init() override;
};

std::string VCMIDirsAndroid::libraryName(const std::string & basename) const { return "lib" + basename + ".so"; }
bfs::path VCMIDirsAndroid::binaryPath() const { return "."; }
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
	return {
		internalPath,
		userDataPath(),
	};
}

void VCMIDirsAndroid::init()
{
	// asks java code to retrieve needed paths from environment
	CAndroidVMHelper envHelper;
	basePath = envHelper.callStaticStringMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "dataRoot");
	internalPath = envHelper.callStaticStringMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "internalDataRoot");
	nativePath = envHelper.callStaticStringMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "nativePath");
	IVCMIDirsUNIX::init();
}

#elif defined(VCMI_PORTMASTER)
class VCMIDirsPM : public IVCMIDirsUNIX
{
public:
	bfs::path userDataPath() const override;
	bfs::path userCachePath() const override;
	bfs::path userConfigPath() const override;

	std::vector<bfs::path> dataPaths() const override;

	bfs::path libraryPath() const override;
	bfs::path binaryPath() const override;

	std::string libraryName(const std::string& basename) const override;
};

bfs::path VCMIDirsPM::userDataPath() const
{
	const char* homeDir;
	if((homeDir = getenv("PORTMASTER_HOME")))
		return bfs::path(homeDir) / "data";
	else
		return bfs::path(".") / "data";
}
bfs::path VCMIDirsPM::userCachePath() const
{
	// $XDG_CACHE_HOME, default: $HOME/.cache
	const char * tempResult;
	if ((tempResult = getenv("PORTMASTER_HOME")))
		return bfs::path(tempResult) / "cache";
	else
		return bfs::path(".") / "cache";
}
bfs::path VCMIDirsPM::userConfigPath() const
{
	// $XDG_CONFIG_HOME, default: $HOME/.config
	const char * tempResult;
	if ((tempResult = getenv("PORTMASTER_HOME")))
		return bfs::path(tempResult) / "save";
	else
		return bfs::path(".") / "save";
}

std::vector<bfs::path> VCMIDirsPM::dataPaths() const
{
	// $XDG_DATA_DIRS, default: /usr/local/share/:/usr/share/

	// construct list in reverse.
	// in specification first directory has highest priority
	// in vcmi fs last directory has highest priority
	std::vector<bfs::path> ret;
	const char * tempResult;
	if ((tempResult = getenv("PORTMASTER_HOME")))
	{
		ret.push_back(bfs::path(tempResult) / "data");
		ret.push_back(bfs::path(tempResult));
	}

	ret.push_back(bfs::path(".") / "data");
	ret.push_back(bfs::path("."));
	return ret;
}

bfs::path VCMIDirsPM::libraryPath() const
{
	const char * tempResult;
	if ((tempResult = getenv("PORTMASTER_HOME")))
		return bfs::path(tempResult) / "libs";
	else
		return M_LIB_DIR;
}

bfs::path VCMIDirsPM::binaryPath() const
{
	const char * tempResult;
	if ((tempResult = getenv("PORTMASTER_HOME")))
		return bfs::path(tempResult) / "bin";
	else
		return M_BIN_DIR;
}

std::string VCMIDirsPM::libraryName(const std::string& basename) const { return "lib" + basename + ".so"; }

#elif defined(VCMI_XDG)
class VCMIDirsXDG : public IVCMIDirsUNIX
{
public:
	bfs::path userDataPath() const override;
	bfs::path userCachePath() const override;
	bfs::path userConfigPath() const override;

	std::vector<bfs::path> dataPaths() const override;

	bfs::path libraryPath() const override;
	bfs::path binaryPath() const override;

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
	const char * tempResult = getenv("XDG_CONFIG_HOME");
	if (tempResult)
		return bfs::path(tempResult) / "vcmi";
	
	tempResult = getenv("HOME");
	if (tempResult)
		return bfs::path(tempResult) / ".config" / "vcmi";

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
		ret.emplace_back(".");
	}
	else
	{
		ret.emplace_back(M_DATA_DIR);
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

#endif // VCMI_APPLE, VCMI_ANDROID, VCMI_XDG
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
		#elif defined(VCMI_PORTMASTER)
			static VCMIDirsPM singleton;
		#elif defined(VCMI_XDG)
			static VCMIDirsXDG singleton;
		#elif defined(VCMI_MAC)
			static VCMIDirsOSX singleton;
		#elif defined(VCMI_IOS)
			static VCMIDirsIOS singleton;
		#endif

		static std::once_flag flag;
		std::call_once(flag, [] { singleton.init(); });
		return singleton;
	}
}

VCMI_LIB_NAMESPACE_END
