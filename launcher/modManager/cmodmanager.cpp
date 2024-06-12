/*
 * cmodmanager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "cmodmanager.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CZipLoader.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/modding/CModInfo.h"
#include "../../lib/modding/IdentifierStorage.h"

#include "../jsonutils.h"
#include "../launcherdirs.h"

#include <future>

namespace
{
QString detectModArchive(QString path, QString modName, std::vector<std::string> & filesToExtract)
{
	ZipArchive archive(qstringToPath(path));

	filesToExtract = archive.listFiles();

	QString modDirName;

	for(int folderLevel : {0, 1}) //search in subfolder if there is no mod.json in the root
	{
		for(auto file : filesToExtract)
		{
			QString filename = QString::fromUtf8(file.c_str());
			modDirName = filename.section('/', 0, folderLevel);
			
			if(filename == modDirName + "/mod.json")
			{
				return modDirName;
			}
		}
	}

	logGlobal->error("Failed to detect mod path in archive!");
	logGlobal->debug("List of file in archive:");
	for(auto file : filesToExtract)
		logGlobal->debug("%s", file.c_str());
	
	return "";
}
}


CModManager::CModManager(CModList * modList)
	: modList(modList)
{
	loadMods();
	loadModSettings();
}

QString CModManager::settingsPath()
{
	return pathToQString(VCMIDirs::get().userConfigPath() / "modSettings.json");
}

void CModManager::loadModSettings()
{
	modSettings = JsonUtils::JsonFromFile(settingsPath()).toMap();
	modList->setModSettings(modSettings["activeMods"]);
}

void CModManager::resetRepositories()
{
	modList->resetRepositories();
}

void CModManager::loadRepositories(QVector<QVariantMap> repomap)
{
	for (auto const & entry : repomap)
		modList->addRepository(entry);
	modList->reloadRepositories();
}

void CModManager::loadMods()
{
	CModHandler handler;
	handler.loadMods();
	auto installedMods = handler.getAllMods();
	localMods.clear();

	for(auto modname : installedMods)
	{
		auto resID = CModInfo::getModFile(modname);
		if(CResourceHandler::get()->existsResource(resID))
		{
			//calculate mod size
			qint64 total = 0;
			ResourcePath resDir(CModInfo::getModDir(modname), EResType::DIRECTORY);
			if(CResourceHandler::get()->existsResource(resDir))
			{
				for(QDirIterator iter(QString::fromStdString(CResourceHandler::get()->getResourceName(resDir)->string()), QDirIterator::Subdirectories); iter.hasNext(); iter.next())
					total += iter.fileInfo().size();
			}
			
			boost::filesystem::path name = *CResourceHandler::get()->getResourceName(resID);
			auto mod = JsonUtils::JsonFromFile(pathToQString(name));
			auto json = JsonUtils::toJson(mod);
			json["localSizeBytes"].Float() = total;
			if(!name.is_absolute())
				json["storedLocaly"].Bool() = true;

			mod = JsonUtils::toVariant(json);
			localMods.insert(QString::fromUtf8(modname.c_str()).toLower(), mod);
		}
	}
	modList->setLocalModList(localMods);
}

bool CModManager::addError(QString modname, QString message)
{
	recentErrors.push_back(QString("%1: %2").arg(modname).arg(message));
	return false;
}

QStringList CModManager::getErrors()
{
	QStringList ret = recentErrors;
	recentErrors.clear();
	return ret;
}

bool CModManager::installMod(QString modname, QString archivePath)
{
	return canInstallMod(modname) && doInstallMod(modname, archivePath);
}

bool CModManager::uninstallMod(QString modname)
{
	return canUninstallMod(modname) && doUninstallMod(modname);
}

bool CModManager::enableMod(QString modname)
{
	return canEnableMod(modname) && doEnableMod(modname, true);
}

bool CModManager::disableMod(QString modname)
{
	return canDisableMod(modname) && doEnableMod(modname, false);
}

bool CModManager::canInstallMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if(mod.isSubmod())
		return addError(modname, tr("Can not install submod"));

	if(mod.isInstalled())
		return addError(modname, tr("Mod is already installed"));
	return true;
}

bool CModManager::canUninstallMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if(mod.isSubmod())
		return addError(modname, tr("Can not uninstall submod"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod is not installed"));

	return true;
}

bool CModManager::canEnableMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if(mod.isEnabled())
		return addError(modname, tr("Mod is already enabled"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod must be installed first"));

	//check for compatibility
	if(!mod.isCompatible())
		return addError(modname, tr("Mod is not compatible, please update VCMI and checkout latest mod revisions"));

	for(auto modEntry : mod.getDependencies())
	{
		if(!modList->hasMod(modEntry)) // required mod is not available
			return addError(modname, tr("Required mod %1 is missing").arg(modEntry));

		CModEntry modData = modList->getMod(modEntry);

		if(!modData.isCompatibilityPatch() && !modData.isEnabled())
			return addError(modname, tr("Required mod %1 is not enabled").arg(modEntry));
	}

	for(QString modEntry : modList->getModList())
	{
		auto mod = modList->getMod(modEntry);

		// "reverse conflict" - enabled mod has this one as conflict
		if(mod.isEnabled() && mod.getConflicts().contains(modname))
			return addError(modname, tr("This mod conflicts with %1").arg(modEntry));
	}

	for(auto modEntry : mod.getConflicts())
	{
		// check if conflicting mod installed and enabled
		if(modList->hasMod(modEntry) && modList->getMod(modEntry).isEnabled())
			return addError(modname, tr("This mod conflicts with %1").arg(modEntry));
	}
	return true;
}

bool CModManager::canDisableMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if(mod.isDisabled())
		return addError(modname, tr("Mod is already disabled"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod must be installed first"));

	for(QString modEntry : modList->getModList())
	{
		auto current = modList->getMod(modEntry);

		if(current.getDependencies().contains(modname) && current.isEnabled())
			return addError(modname, tr("This mod is needed to run %1").arg(modEntry));
	}
	return true;
}

static QVariant writeValue(QString path, QVariantMap input, QVariant value)
{
	if(path.size() > 1)
	{

		QString entryName = path.section('/', 0, 1);
		QString remainder = "/" + path.section('/', 2, -1);

		entryName.remove(0, 1);
		input.insert(entryName, writeValue(remainder, input.value(entryName).toMap(), value));
		return input;
	}
	else
	{
		return value;
	}
}

bool CModManager::doEnableMod(QString mod, bool on)
{
	QString path = mod;
	path = "/activeMods/" + path.replace(".", "/mods/") + "/active";

	modSettings = writeValue(path, modSettings, QVariant(on)).toMap();
	modList->setModSettings(modSettings["activeMods"]);
	modList->modChanged(mod);

	JsonUtils::JsonToFile(settingsPath(), modSettings);

	return true;
}

bool CModManager::doInstallMod(QString modname, QString archivePath)
{
	const auto destDir = CLauncherDirs::modsPath() + QChar{'/'};

	if(!QFile(archivePath).exists())
		return addError(modname, tr("Mod archive is missing"));

	if(localMods.contains(modname))
		return addError(modname, tr("Mod with such name is already installed"));

	std::vector<std::string> filesToExtract;
	QString modDirName = ::detectModArchive(archivePath, modname, filesToExtract);
	if(!modDirName.size())
		return addError(modname, tr("Mod archive is invalid or corrupted"));
	
	std::atomic<int> filesCounter = 0;

	auto futureExtract = std::async(std::launch::async, [&archivePath, &destDir, &filesCounter, &filesToExtract]()
	{
		const auto destDirFsPath = qstringToPath(destDir);
		ZipArchive archive(qstringToPath(archivePath));
		for (auto const & file : filesToExtract)
		{
			if (!archive.extract(destDirFsPath, file))
				return false;
			++filesCounter;
		}
		return true;
	});
	
	while(futureExtract.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)
	{
		emit extractionProgress(filesCounter, filesToExtract.size());
		qApp->processEvents();
	}
	
	if(!futureExtract.get())
	{
		removeModDir(destDir + modDirName);
		return addError(modname, tr("Failed to extract mod data"));
	}

	//rename folder and fix the path
	QDir extractedDir(destDir + modDirName);
	auto rc = QFile::rename(destDir + modDirName, destDir + modname);
	if (rc)
		extractedDir.setPath(destDir + modname);
	
	//there are possible excessive files - remove them
	QString upperLevel = modDirName.section('/', 0, 0);
	if(upperLevel != modDirName)
		removeModDir(destDir + upperLevel);
	
	CResourceHandler::get("initial")->updateFilteredFiles([](const std::string &) { return true; });
	loadMods();
	modList->reloadRepositories();

	return true;
}

bool CModManager::doUninstallMod(QString modname)
{
	ResourcePath resID(std::string("Mods/") + modname.toStdString(), EResType::DIRECTORY);
	// Get location of the mod, in case-insensitive way
	QString modDir = pathToQString(*CResourceHandler::get()->getResourceName(resID));

	if(!QDir(modDir).exists())
		return addError(modname, tr("Data with this mod was not found"));

	QDir modFullDir(modDir);
	if(!removeModDir(modDir))
		return addError(modname, tr("Mod is located in protected directory, please remove it manually:\n") + modFullDir.absolutePath());

	CResourceHandler::get("initial")->updateFilteredFiles([](const std::string &){ return true; });
	loadMods();
	modList->reloadRepositories();

	return true;
}

bool CModManager::removeModDir(QString path)
{
	// issues 2673 and 2680 its why you do not recursively remove without sanity check
	QDir checkDir(path);
	QDir dir(path);
	
	if(!checkDir.cdUp() || QString::compare("Mods", checkDir.dirName(), Qt::CaseInsensitive))
		return false;
#ifndef VCMI_MOBILE // ios and android applications are stored in the isolated container
	if(!checkDir.cdUp() || QString::compare("vcmi", checkDir.dirName(), Qt::CaseInsensitive))
		return false;

	if(!dir.absolutePath().contains("vcmi", Qt::CaseInsensitive))
		return false;
#endif
	if(!dir.absolutePath().contains("Mods", Qt::CaseInsensitive))
		return false;

	return dir.removeRecursively();
}
