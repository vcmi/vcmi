/*
 * cmodlist.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "cmodlist.h"

#include "../lib/CConfigHandler.h"
#include "../../lib/filesystem/CFileInputStream.h"
#include "../../lib/GameConstants.h"
#include "../../lib/modding/CModVersion.h"

QString CModEntry::sizeToString(double size)
{
	static const std::array sizes {
		QT_TRANSLATE_NOOP("File size", "%1 B"),
		QT_TRANSLATE_NOOP("File size", "%1 KiB"),
		QT_TRANSLATE_NOOP("File size", "%1 MiB"),
		QT_TRANSLATE_NOOP("File size", "%1 GiB"),
		QT_TRANSLATE_NOOP("File size", "%1 TiB")
	};
	size_t index = 0;
	while(size > 1024 && index < sizes.size())
	{
		size /= 1024;
		index++;
	}
	return QCoreApplication::translate("File size", sizes[index]).arg(QString::number(size, 'f', 1));
}

CModEntry::CModEntry(QVariantMap repository, QVariantMap localData, QVariantMap modSettings, QString modname)
	: repository(repository), localData(localData), modSettings(modSettings), modname(modname)
{
}

bool CModEntry::isEnabled() const
{
	if(!isInstalled())
		return false;

	if (!isVisible())
		return false;

	return modSettings["active"].toBool();
}

bool CModEntry::isDisabled() const
{
	if(!isInstalled())
		return false;
	return !isEnabled();
}

bool CModEntry::isAvailable() const
{
	if(isInstalled())
		return false;
	return !repository.isEmpty();
}

bool CModEntry::isUpdateable() const
{
	if(!isInstalled())
		return false;

	auto installedVer = localData["installedVersion"].toString().toStdString();
	auto availableVer = repository["latestVersion"].toString().toStdString();

	return (CModVersion::fromString(installedVer) < CModVersion::fromString(availableVer));
}

bool isCompatible(const QVariantMap & compatibility)
{
	auto compatibleMin = CModVersion::fromString(compatibility["min"].toString().toStdString());
	auto compatibleMax = CModVersion::fromString(compatibility["max"].toString().toStdString());

	return (compatibleMin.isNull() || CModVersion::GameVersion().compatible(compatibleMin, true, true))
			&& (compatibleMax.isNull() || compatibleMax.compatible(CModVersion::GameVersion(), true, true));
}

bool CModEntry::isCompatible() const
{
	return ::isCompatible(localData["compatibility"].toMap());
}

bool CModEntry::isEssential() const
{
	return getName() == "vcmi";
}

bool CModEntry::isInstalled() const
{
	return !localData.isEmpty();
}

bool CModEntry::isVisible() const
{
	if (isCompatibilityPatch())
	{
		if (isSubmod())
			return false;
	}

	if (isTranslation())
	{
		// Do not show not installed translation mods to languages other than player language
		if (localData.empty() && getBaseValue("language") != QString::fromStdString(settings["general"]["language"].String()) )
			return false;
	}

	return !localData.isEmpty() || (!repository.isEmpty() && !repository.contains("mod"));
}

bool CModEntry::isTranslation() const
{
	return getBaseValue("modType").toString() == "Translation";
}

bool CModEntry::isCompatibilityPatch() const
{
	return getBaseValue("modType").toString() == "Compatibility";
}

bool CModEntry::isSubmod() const
{
	return getName().contains('.');
}

int CModEntry::getModStatus() const
{
	int status = 0;
	if(isEnabled())
		status |= ModStatus::ENABLED;
	if(isInstalled())
		status |= ModStatus::INSTALLED;
	if(isUpdateable())
		status |= ModStatus::UPDATEABLE;

	return status;
}

QString CModEntry::getName() const
{
	return modname;
}

QVariant CModEntry::getValue(QString value) const
{
	return getValueImpl(value, true);
}

QStringList CModEntry::getDependencies() const
{
	QStringList result;
	for (auto const & entry : getValue("depends").toStringList())
		result.push_back(entry.toLower());
	return result;
}

QStringList CModEntry::getConflicts() const
{
	QStringList result;
	for (auto const & entry : getValue("conflicts").toStringList())
		result.push_back(entry.toLower());
	return result;
}

QVariant CModEntry::getBaseValue(QString value) const
{
	return getValueImpl(value, false);
}

QVariant CModEntry::getValueImpl(QString value, bool localized) const

{
	QString langValue = QString::fromStdString(settings["general"]["language"].String());

	// Priorities
	// 1) data from newest version
	// 2) data from preferred language

	bool useRepositoryData = repository.contains(value);

	if(repository.contains(value) && localData.contains(value))
	{
		// value is present in both repo and locally installed. Select one from latest version
		auto installedVer = localData["installedVersion"].toString().toStdString();
		auto availableVer = repository["latestVersion"].toString().toStdString();

		useRepositoryData = CModVersion::fromString(installedVer) < CModVersion::fromString(availableVer);
	}

	auto & storage = useRepositoryData ? repository : localData;

	if(localized && storage.contains(langValue))
	{
		auto langStorage = storage[langValue].toMap();
		if (langStorage.contains(value))
			return langStorage[value];
	}

	if(storage.contains(value))
		return storage[value];

	return QVariant();
}

QVariantMap CModList::copyField(QVariantMap data, QString from, QString to) const
{
	QVariantMap renamed;

	for(auto it = data.begin(); it != data.end(); it++)
	{
		QVariantMap modConf = it.value().toMap();

		modConf.insert(to, modConf.value(from));
		renamed.insert(it.key(), modConf);
	}
	return renamed;
}

void CModList::reloadRepositories()
{
	cachedMods.clear();
}

void CModList::resetRepositories()
{
	repositories.clear();
	cachedMods.clear();
}

void CModList::addRepository(QVariantMap data)
{
	for(auto & key : data.keys())
		data[key.toLower()] = data.take(key);
	repositories.push_back(copyField(data, "version", "latestVersion"));

	cachedMods.clear();
}

void CModList::setLocalModList(QVariantMap data)
{
	localModList = copyField(data, "version", "installedVersion");
	cachedMods.clear();
}

void CModList::setModSettings(QVariant data)
{
	modSettings = data.toMap();
	cachedMods.clear();
}

void CModList::modChanged(QString modID)
{
	cachedMods.clear();
}

static QVariant getValue(QVariant input, QString path)
{
	if(path.size() > 1)
	{
		QString entryName = path.section('/', 0, 1);
		QString remainder = "/" + path.section('/', 2, -1);

		entryName.remove(0, 1);
		return getValue(input.toMap().value(entryName), remainder);
	}
	else
	{
		return input;
	}
}

const CModEntry & CModList::getMod(QString modName) const
{
	modName = modName.toLower();

	auto it = cachedMods.find(modName);

	if (it != cachedMods.end())
		return it.value();

	auto itNew = cachedMods.insert(modName, getModUncached(modName));
	return *itNew;
}

CModEntry CModList::getModUncached(QString modname) const
{
	QVariantMap repo;
	QVariantMap local = localModList[modname].toMap();
	QVariantMap settings;

	QString path = modname;
	path = "/" + path.replace(".", "/mods/");
	QVariant conf = getValue(modSettings, path);

	if(conf.isNull())
	{
		settings["active"] = !local.value("keepDisabled").toBool();
	}
	else
	{
		if(!conf.toMap().isEmpty())
		{
			settings = conf.toMap();
			if(settings.value("active").isNull())
				settings["active"] = !local.value("keepDisabled").toBool();
		}
		else
			settings.insert("active", conf);
	}
	
	if(settings["active"].toBool())
	{
		QString rootPath = path.section('/', 0, 1);
		if(path != rootPath)
		{
			conf = getValue(modSettings, rootPath);
			const auto confMap = conf.toMap();
			if(!conf.isNull() && !confMap["active"].isNull() && !confMap["active"].toBool())
			{
				settings = confMap;
			}
		}
	}

	if(settings.value("active").toBool())
	{
		if(!::isCompatible(local.value("compatibility").toMap()))
			settings["active"] = false;
	}

	for(auto entry : repositories)
	{
		QVariant repoVal = getValue(entry, path);
		if(repoVal.isValid())
		{
			auto repoValMap = repoVal.toMap();
			if(::isCompatible(repoValMap["compatibility"].toMap()))
			{
				if(repo.empty()
					|| CModVersion::fromString(repo["version"].toString().toStdString())
					 < CModVersion::fromString(repoValMap["version"].toString().toStdString()))
				{
					//take valid download link, screenshots and mod size before assignment
					auto download = repo.value("download");
					auto screenshots = repo.value("screenshots");
					auto size = repo.value("downloadSize");
					repo = repoValMap;
					if(repo.value("download").isNull())
					{
						repo["download"] = download;
						if(repo.value("screenshots").isNull()) //taking screenshot from the downloadable version
							repo["screenshots"] = screenshots;
					}
					if(repo.value("downloadSize").isNull())
						repo["downloadSize"] = size;
				}
			}
		}
	}

	return CModEntry(repo, local, settings, modname);
}

bool CModList::hasMod(QString modname) const
{
	if(localModList.contains(modname))
		return true;

	for(auto entry : repositories)
		if(entry.contains(modname))
			return true;

	return false;
}

QStringList CModList::getRequirements(QString modname)
{
	QStringList ret;

	if(hasMod(modname))
	{
		auto mod = getMod(modname);

		for(auto entry : mod.getDependencies())
			ret += getRequirements(entry.toLower());
	}
	ret += modname;

	return ret;
}

QVector<QString> CModList::getModList() const
{
	QSet<QString> knownMods;
	QVector<QString> modList;
	for(auto repo : repositories)
	{
		for(auto it = repo.begin(); it != repo.end(); it++)
		{
			knownMods.insert(it.key().toLower());
		}
	}
	for(auto it = localModList.begin(); it != localModList.end(); it++)
	{
		knownMods.insert(it.key().toLower());
	}

	for(auto entry : knownMods)
	{
		modList.push_back(entry);
	}
	return modList;
}

QVector<QString> CModList::getChildren(QString parent) const
{
	QVector<QString> children;

	int depth = parent.count('.') + 1;
	for(const QString & mod : getModList())
	{
		if(mod.count('.') == depth && mod.startsWith(parent))
			children.push_back(mod);
	}
	return children;
}
