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
#include "../../lib/json/JsonNode.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include "../../vcmiqt/launcherdirs.h"

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

bool ModStateController::canUninstallMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if(mod.isSubmod())
		return addError(modname, tr("Can not uninstall submod"));

	if(!mod.isInstalled())
		return addError(modname, tr("Mod is not installed"));

	if(!isModManageable(modname))
		return addError(modname, getUnmanageableReason(modname));

	return true;
}

QString ModStateController::getUnmanageableReason(const QString & modname)
{
	const QString modDirectory = findModDirectory(modname.section('.', 0, 0));

	if(modDirectory.isEmpty())
		return tr("Mod is not located in user data directory and can not be managed by launcher");

	// .git may also be a file, in case of git worktrees and submodules
	if(QFileInfo::exists(modDirectory + "/.git"))
		return tr("Mod directory is a git repository and can not be managed by launcher");

	return {};
}

bool ModStateController::isModManageable(QString modname)
{
	return getUnmanageableReason(modname).isEmpty();
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

QString ModStateController::findModDirectory(const QString & modname)
{
	const QDir modsDir(CLauncherDirs::modsPath());
	for(const auto & entry : modsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
		if(entry.compare(modname, Qt::CaseInsensitive) == 0)
			return modsDir.filePath(entry);
	return {};
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
