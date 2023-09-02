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
#include "../../lib/JsonNode.h"
#include "../../lib/filesystem/CFileInputStream.h"
#include "../../lib/GameConstants.h"
#include "../../lib/modding/CModVersion.h"

QString CModEntry::sizeToString(double size)
{
	static const QString sizes[] =
	{
		/*"%1 B", */ "%1 KiB", "%1 MiB", "%1 GiB", "%1 TiB"
	};
	size_t index = 0;
	while(size > 1024 && index < 4)
	{
		size /= 1024;
		index++;
	}
	return sizes[index].arg(QString::number(size, 'f', 1));
}

CModEntry::CModEntry(QVariantMap repository, QVariantMap localData, QVariantMap modSettings, QString modname)
	: repository(repository), localData(localData), modSettings(modSettings), modname(modname)
{
}

bool CModEntry::isEnabled() const
{
	if(!isInstalled())
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

bool CModEntry::isValid() const
{
	return !localData.isEmpty() || !repository.isEmpty();
}

bool CModEntry::isTranslation() const
{
	return getBaseValue("modType").toString().toLower() == "translation";
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

QVariantMap CModList::copyField(QVariantMap data, QString from, QString to)
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
}

void CModList::resetRepositories()
{
	repositories.clear();
}

void CModList::addRepository(QVariantMap data)
{
	for(auto & key : data.keys())
		data[key.toLower()] = data.take(key);
	repositories.push_back(copyField(data, "version", "latestVersion"));
}

void CModList::setLocalModList(QVariantMap data)
{
	localModList = copyField(data, "version", "installedVersion");
}

void CModList::setModSettings(QVariant data)
{
	modSettings = data.toMap();
}

void CModList::modChanged(QString modID)
{
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

CModEntry CModList::getMod(QString modname) const
{
	modname = modname.toLower();
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
					//take valid download link and screenshots before assignment
					auto download = repo.value("download");
					auto screenshots = repo.value("screenshots");
					repo = repoValMap;
					if(repo.value("download").isNull())
					{
						repo["download"] = download;
						if(repo.value("screenshots").isNull()) //taking screenshot from the downloadable version
							repo["screenshots"] = screenshots;
					}
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

		for(auto entry : mod.getValue("depends").toStringList())
			ret += getRequirements(entry);
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
