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

#include "../../lib/JsonNode.h"
#include "../../lib/filesystem/CFileInputStream.h"
#include "../../lib/GameConstants.h"

namespace
{
bool isCompatible(const QString & verMin, const QString & verMax)
{
	const int maxSections = 3; // versions consist from up to 3 sections, major.minor.patch
	QVersionNumber vcmiVersion(GameConstants::VCMI_VERSION_MAJOR,
							   GameConstants::VCMI_VERSION_MINOR,
							   GameConstants::VCMI_VERSION_PATCH);
	
	auto versionMin = QVersionNumber::fromString(verMin);
	auto versionMax = QVersionNumber::fromString(verMax);
	
	auto buildVersion = [maxSections](QVersionNumber & ver)
	{
		if(ver.segmentCount() < maxSections)
		{
			auto segments = ver.segments();
			for(int i = segments.size() - 1; i < maxSections; ++i)
				segments.append(0);
			ver = QVersionNumber(segments);
		}
	};

	if(!versionMin.isNull())
	{
		buildVersion(versionMin);
		if(vcmiVersion < versionMin)
			return false;
	}
	
	if(!versionMax.isNull())
	{
		buildVersion(versionMax);
		if(vcmiVersion > versionMax)
			return false;
	}
	return true;
}
}

bool CModEntry::compareVersions(QString lesser, QString greater)
{
	auto versionLesser = QVersionNumber::fromString(lesser);
	auto versionGreater = QVersionNumber::fromString(greater);
	return versionLesser < versionGreater;
}

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

	QString installedVer = localData["installedVersion"].toString();
	QString availableVer = repository["latestVersion"].toString();

	if(compareVersions(installedVer, availableVer))
		return true;
	return false;
}

bool CModEntry::isCompatible() const
{
	if(!isInstalled())
		return false;

	auto compatibility = localData["compatibility"].toMap();
	return ::isCompatible(compatibility["min"].toString(), compatibility["max"].toString());
}

bool CModEntry::isEssential() const
{
	return getValue("storedLocaly").toBool();
}

bool CModEntry::isInstalled() const
{
	return !localData.isEmpty();
}

bool CModEntry::isValid() const
{
	return !localData.isEmpty() || !repository.isEmpty();
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
	if(repository.contains(value) && localData.contains(value))
	{
		// value is present in both repo and locally installed. Select one from latest version
		QString installedVer = localData["installedVersion"].toString();
		QString availableVer = repository["latestVersion"].toString();

		if(compareVersions(installedVer, availableVer))
			return repository[value];
		else
			return localData[value];
	}

	if(repository.contains(value))
		return repository[value];

	if(localData.contains(value))
		return localData[value];

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
		settings["active"] = true; // default
	}
	else
	{
		if(!conf.toMap().isEmpty())
		{
			settings = conf.toMap();
			if(settings.value("active").isNull())
				settings["active"] = true; // default
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
		auto compatibility = local.value("compatibility").toMap();
		if(compatibility["min"].isValid() || compatibility["max"].isValid())
			if(!isCompatible(compatibility["min"].toString(), compatibility["max"].toString()))
				settings["active"] = false;
	}


	for(auto entry : repositories)
	{
		QVariant repoVal = getValue(entry, path);
		if(repoVal.isValid())
		{
			auto repoValMap = repoVal.toMap();
			auto compatibility = repoValMap["compatibility"].toMap();
			if(isCompatible(compatibility["min"].toString(), compatibility["max"].toString()))
			{
				if(repo.empty() || CModEntry::compareVersions(repo["version"].toString(), repoValMap["version"].toString()))
				{
					repo = repoValMap;
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
