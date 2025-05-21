/*
 * modstatecontroller.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "modstatecontroller.h"

#include "modstatemodel.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CZipLoader.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include "../vcmiqt/jsonutils.h"
#include "../vcmiqt/launcherdirs.h"

#include <future>

namespace
{
QString detectModArchive(QString path, QString modName, std::vector<std::string> & filesToExtract)
{
	try {
		ZipArchive archive(qstringToPath(path));
		filesToExtract = archive.listFiles();
	}
	catch (const std::runtime_error & e)
	{
		logGlobal->error("Failed to open zip archive. Reason: %s", e.what());
		return "";
	}

	QString modDirName;

	for(int folderLevel : {0, 1}) //search in subfolder if there is no mod.json in the root
	{
		for(const auto & file : filesToExtract)
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
	for(const auto & file : filesToExtract)
		logGlobal->debug("%s", file.c_str());
	
	return "";
}
}


ModStateController::ModStateController(std::shared_ptr<ModStateModel> modList)
	: modList(modList)
{
}

ModStateController::~ModStateController() = default;

void ModStateController::setRepositoryData(const JsonNode & repomap)
{
	modList->setRepositoryData(repomap);
}

bool ModStateController::addError(QString modname, QString message)
{
	recentErrors.push_back(QString("%1: %2").arg(modname).arg(message));
	return false;
}

QStringList ModStateController::getErrors()
{
	QStringList ret = recentErrors;
	recentErrors.clear();
	return ret;
}

bool ModStateController::installMod(QString modname, QString archivePath)
{
	return canInstallMod(modname) && doInstallMod(modname, archivePath);
}

bool ModStateController::uninstallMod(QString modname)
{
	return canUninstallMod(modname) && doUninstallMod(modname);
}

bool ModStateController::enableMods(QStringList modlist)
{
	for (const auto & modname : modlist)
		if (!canEnableMod(modname))
			return false;

	modList->doEnableMods(modlist);
	return true;
}

bool ModStateController::disableMod(QString modname)
{
	if (!canDisableMod(modname))
		return false;
	modList->doDisableMod(modname);
	return true;
}

bool ModStateController::canInstallMod(QString modname)
{
	if (!modList->isModExists(modname))
		return true; // for installation of unknown mods, e.g. via "Install from file" option

	auto mod = modList->getMod(modname);

	if(mod.isSubmod())
		return addError(modname, tr("Can not install submod"));

	if(mod.isInstalled())
		return addError(modname, tr("Mod is already installed"));
	return true;
}

bool ModStateController::canUninstallMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if(mod.isSubmod())
		return addError(modname, tr("Can not uninstall submod"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod is not installed"));

	return true;
}

bool ModStateController::canEnableMod(QString modname)
{
	if (!modList->isModExists(modname))
		return false;

	auto mod = modList->getMod(modname);

	if(modList->isModEnabled(modname))
		return addError(modname, tr("Mod is already enabled"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod must be installed first"));

	//check for compatibility
	if(!mod.isCompatible())
		return addError(modname, tr("Mod is not compatible, please update VCMI and check the latest mod revisions"));

	if (mod.isTranslation() && CGeneralTextHandler::getPreferredLanguage() != mod.getBaseLanguage().toStdString())
		return addError(modname, tr("Can not enable translation mod for a different language!"));

	for(const auto & modEntry : mod.getDependencies())
	{
		if(!modList->isModExists(modEntry)) // required mod is not available
			return addError(modname, tr("Required mod %1 is missing").arg(modEntry));
	}

	return true;
}

bool ModStateController::canDisableMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if(!modList->isModEnabled(modname))
		return addError(modname, tr("Mod is already disabled"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod must be installed first"));

	return true;
}

bool ModStateController::doInstallMod(QString modname, QString archivePath)
{
	const auto destDir = CLauncherDirs::modsPath() + QChar{'/'};

	if(!QFile(archivePath).exists())
		return addError(modname, tr("Mod archive is missing"));

	std::vector<std::string> filesToExtract;
	QString modDirName = ::detectModArchive(archivePath, modname, filesToExtract);
	if(!modDirName.size())
		return addError(modname, tr("Mod archive is invalid or corrupted"));
	
	std::atomic<int> filesCounter = 0;

	auto futureExtract = std::async(std::launch::async, [&archivePath, &destDir, &filesCounter, &filesToExtract]()
	{
		const auto destDirFsPath = qstringToPath(destDir);
		ZipArchive archive(qstringToPath(archivePath));
		for(const auto & file : filesToExtract)
		{
			if (!archive.extract(destDirFsPath, file))
				return false;
			++filesCounter;
		}
		return true;
	});
	
	while(futureExtract.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)
	{
		extractionProgress(filesCounter, filesToExtract.size());
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

	// Remove .github folder from installed mod
	QDir githubDir(extractedDir.filePath(".github"));
	if (githubDir.exists())
	    githubDir.removeRecursively();
	
	//there are possible excessive files - remove them
	QString upperLevel = modDirName.section('/', 0, 0);
	if(upperLevel != modDirName)
		removeModDir(destDir + upperLevel);

	return true;
}

bool ModStateController::doUninstallMod(QString modname)
{
	ResourcePath resID(std::string("Mods/") + modname.toStdString(), EResType::DIRECTORY);
	// Get location of the mod, in case-insensitive way
	QString modDir = pathToQString(*CResourceHandler::get()->getResourceName(resID));

	if(!QDir(modDir).exists())
		return addError(modname, tr("Mod data was not found"));

	QDir modFullDir(modDir);
	if(!removeModDir(modDir))
		return addError(modname, tr("Mod is located in a protected directory, please remove it manually:\n") + modFullDir.absolutePath());

	return true;
}

bool ModStateController::removeModDir(QString path)
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
