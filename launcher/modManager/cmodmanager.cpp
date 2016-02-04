#include "StdInc.h"
#include "cmodmanager.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CZipLoader.h"
#include "../../lib/CModHandler.h"

#include "../jsonutils.h"
#include "../launcherdirs.h"

static QString detectModArchive(QString path, QString modName)
{
	auto files = ZipArchive::listFiles(qstringToPath(path));

	QString modDirName;

	for (auto file : files)
	{
		QString filename = QString::fromUtf8(file.c_str());
		if (filename.toLower().startsWith(modName))
		{
			// archive must contain mod.json file
			if (filename.toLower() == modName + "/mod.json")
				modDirName = filename.section('/', 0, 0);
		}
		else // all files must be in <modname> directory
			return "";
	}
	return modDirName;
}

CModManager::CModManager(CModList * modList):
    modList(modList)
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

void CModManager::loadRepository(QString file)
{
	modList->addRepository(JsonUtils::JsonFromFile(file).toMap());
}

void CModManager::loadMods()
{
	CModHandler handler;
	handler.loadMods();
	auto installedMods = handler.getAllMods();

	for (auto modname : installedMods)
	{
		ResourceID resID(CModInfo::getModFile(modname));
		if (CResourceHandler::get()->existsResource(resID))
		{
			boost::filesystem::path name = *CResourceHandler::get()->getResourceName(resID);
			auto mod = JsonUtils::JsonFromFile(pathToQString(name));
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

	if (mod.getName().contains('.'))
		return addError(modname, "Can not install submod");

	if (mod.isInstalled())
		return addError(modname, "Mod is already installed");

	if (!mod.isAvailable())
		return addError(modname, "Mod is not available");
	return true;
}

bool CModManager::canUninstallMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if (mod.getName().contains('.'))
		return addError(modname, "Can not uninstall submod");

	if (!mod.isInstalled())
		return addError(modname, "Mod is not installed");

	if (mod.isEnabled())
		return addError(modname, "Mod must be disabled first");
	return true;
}

bool CModManager::canEnableMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if (mod.isEnabled())
		return addError(modname, "Mod is already enabled");

	if (!mod.isInstalled())
		return addError(modname, "Mod must be installed first");

	for (auto modEntry : mod.getValue("depends").toStringList())
	{
		if (!modList->hasMod(modEntry)) // required mod is not available
			return addError(modname, QString("Required mod %1 is missing").arg(modEntry));
		if (!modList->getMod(modEntry).isEnabled())
			return addError(modname, QString("Required mod %1 is not enabled").arg(modEntry));
	}

	for (QString modEntry : modList->getModList())
	{
		auto mod = modList->getMod(modEntry);

		// "reverse conflict" - enabled mod has this one as conflict
		if (mod.isEnabled() && mod.getValue("conflicts").toStringList().contains(modname))
			return addError(modname, QString("This mod conflicts with %1").arg(modEntry));
	}

	for (auto modEntry : mod.getValue("conflicts").toStringList())
	{
		if (modList->hasMod(modEntry) &&
		    modList->getMod(modEntry).isEnabled()) // conflicting mod installed and enabled
			return addError(modname, QString("This mod conflicts with %1").arg(modEntry));
	}
	return true;
}

bool CModManager::canDisableMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if (mod.isDisabled())
		return addError(modname, "Mod is already disabled");

	if (!mod.isInstalled())
		return addError(modname, "Mod must be installed first");

	for (QString modEntry : modList->getModList())
	{
		auto current = modList->getMod(modEntry);

		if (current.getValue("depends").toStringList().contains(modname) &&
		    current.isEnabled())
			return addError(modname, QString("This mod is needed to run %1").arg(modEntry));
	}
	return true;
}

static QVariant writeValue(QString path, QVariantMap input, QVariant value)
{
	if (path.size() > 1)
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
	QString destDir = CLauncherDirs::get().modsPath() + "/";

	if (!QFile(archivePath).exists())
		return addError(modname, "Mod archive is missing");

	if (localMods.contains(modname))
		return addError(modname, "Mod with such name is already installed");

	QString modDirName = detectModArchive(archivePath, modname);
	if (!modDirName.size())
		return addError(modname, "Mod archive is invalid or corrupted");

	if (!ZipArchive::extract(qstringToPath(archivePath), qstringToPath(destDir)))
	{
		QDir(destDir + modDirName).removeRecursively();
		return addError(modname, "Failed to extract mod data");
	}

	QVariantMap json = JsonUtils::JsonFromFile(destDir + modDirName + "/mod.json").toMap();

	localMods.insert(modname, json);
	modList->setLocalModList(localMods);
	modList->modChanged(modname);

	return true;
}

bool CModManager::doUninstallMod(QString modname)
{
	ResourceID resID(std::string("Mods/") + modname.toUtf8().data(), EResType::DIRECTORY);
	// Get location of the mod, in case-insensitive way
	QString modDir = pathToQString(*CResourceHandler::get()->getResourceName(resID));

	if (!QDir(modDir).exists())
		return addError(modname, "Data with this mod was not found");

	if (!localMods.contains(modname))
		return addError(modname, "Data with this mod was not found");

	if (!QDir(modDir).removeRecursively())
		return addError(modname, "Failed to delete mod data");

	localMods.remove(modname);
	modList->setLocalModList(localMods);
	modList->modChanged(modname);

	return true;
}
