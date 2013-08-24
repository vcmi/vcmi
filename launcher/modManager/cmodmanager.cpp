#include "StdInc.h"
#include "cmodmanager.h"

#include "../lib/VCMIDirs.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CZipLoader.h"

#include "../launcherdirs.h"

static QJsonObject JsonFromFile(QString filename)
{
	QFile file(filename);
	file.open(QFile::ReadOnly);

	return QJsonDocument::fromJson(file.readAll()).object();
}

static void JsonToFile(QString filename, QJsonObject object)
{
	QFile file(filename);
	file.open(QFile::WriteOnly);
	file.write(QJsonDocument(object).toJson());
}

static QString detectModArchive(QString path, QString modName)
{
	auto files = ZipArchive::listFiles(path.toUtf8().data());

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
	return QString::fromUtf8(VCMIDirs::get().userConfigPath().c_str()) + "/modSettings.json";
}

void CModManager::loadModSettings()
{
	modSettings = JsonFromFile(settingsPath());
	modList->setModSettings(modSettings["activeMods"].toObject());
}

void CModManager::resetRepositories()
{
	modList->resetRepositories();
}

void CModManager::loadRepository(QString file)
{
	modList->addRepository(JsonFromFile(file));
}

void CModManager::loadMods()
{
	auto installedMods = CResourceHandler::getAvailableMods();

	for (auto modname : installedMods)
	{
		ResourceID resID("Mods/" + modname + "/mod.json");

		if (CResourceHandler::get()->existsResource(resID))
		{
			auto data = CResourceHandler::get()->load(resID)->readAll();
			auto array = QByteArray(reinterpret_cast<char *>(data.first.get()), data.second);

			auto mod = QJsonDocument::fromJson(array);
			assert (mod.isObject()); // TODO: use JsonNode from vcmi code here - QJsonNode parser is just too pedantic

			localMods.insert(QString::fromUtf8(modname.c_str()).toLower(), QJsonValue(mod.object()));
		}
	}
	modList->setLocalModList(localMods);
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

	if (mod.isInstalled())
		return false;

	if (!mod.isAvailable())
		return false;
	return true;
}

bool CModManager::canUninstallMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if (!mod.isInstalled())
		return false;

	if (mod.isEnabled())
		return false;
	return true;
}

bool CModManager::canEnableMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if (mod.isEnabled())
		return false;

	if (!mod.isInstalled())
		return false;

	for (auto modEntry : mod.getValue("depends").toStringList())
	{
		if (!modList->hasMod(modEntry)) // required mod is not available
			return false;
		if (!modList->getMod(modEntry).isEnabled())
			return false;
	}

	for (QString name : modList->getModList())
	{
		auto mod = modList->getMod(name);

		if (mod.isEnabled() && mod.getValue("conflicts").toStringList().contains(modname))
			return false; // "reverse conflict" - enabled mod has this one as conflict
	}

	for (auto modEntry : mod.getValue("conflicts").toStringList())
	{
		if (modList->hasMod(modEntry) &&
		    modList->getMod(modEntry).isEnabled()) // conflicting mod installed and enabled
			return false;
	}
	return true;
}

bool CModManager::canDisableMod(QString modname)
{
	auto mod = modList->getMod(modname);

	if (mod.isDisabled())
		return false;

	if (!mod.isInstalled())
		return false;

	for (QString modEntry : modList->getModList())
	{
		auto current = modList->getMod(modEntry);

		if (current.getValue("depends").toStringList().contains(modname) &&
		    current.isEnabled())
			return false; // this mod must be disabled first
	}
	return true;
}

bool CModManager::doEnableMod(QString mod, bool on)
{
	QJsonValue value(on);
	QJsonObject list = modSettings["activeMods"].toObject();

	list.insert(mod, value);
	modSettings.insert("activeMods", list);

	modList->setModSettings(modSettings["activeMods"].toObject());

	JsonToFile(settingsPath(), modSettings);

	return true;
}

bool CModManager::doInstallMod(QString modname, QString archivePath)
{
	QString destDir = CLauncherDirs::get().modsPath() + "/";

	if (!QFile(archivePath).exists())
		return false; // archive with mod data exists

	if (QDir(destDir + modname).exists()) // FIXME: recheck wog/vcmi data behavior - they have bits of data in our trunk
		return false; // no mod with such name installed

	if (localMods.contains(modname))
		return false; // no installed data known

	QString modDirName = detectModArchive(archivePath, modname);
	if (!modDirName.size())
		return false; // archive content looks like mod FS

	if (!ZipArchive::extract(archivePath.toUtf8().data(), destDir.toUtf8().data()))
	{
		QDir(destDir + modDirName).removeRecursively();
		return false; // extraction failed
	}

	QJsonObject json = JsonFromFile(destDir + modDirName + "/mod.json");

	localMods.insert(modname, json);
	modList->setLocalModList(localMods);

	return true;
}

bool CModManager::doUninstallMod(QString modname)
{
	ResourceID resID(std::string("Mods/") + modname.toUtf8().data(), EResType::DIRECTORY);
	// Get location of the mod, in case-insensitive way
	QString modDir = QString::fromUtf8(CResourceHandler::get()->getResourceName(resID)->c_str());

	if (!QDir(modDir).exists())
		return false;

	if (!localMods.contains(modname))
		return false;

	if (!QDir(modDir).removeRecursively())
		return false;

	localMods.remove(modname);
	modList->setLocalModList(localMods);

	return true;
}
