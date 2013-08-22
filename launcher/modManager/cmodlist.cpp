#include "StdInc.h"
#include "cmodlist.h"

bool CModEntry::compareVersions(QString lesser, QString greater)
{
	static const int maxSections = 3; // versions consist from up to 3 sections, major.minor.patch

	QStringList lesserList = lesser.split(".");
	QStringList greaterList = greater.split(".");

	assert(lesserList.size() <= maxSections);
	assert(greaterList.size() <= maxSections);

	for (int i=0; i< maxSections; i++)
	{
		if (greaterList.size() <= i) // 1.1.1 > 1.1
			return false;

		if (lesserList.size() <= i) // 1.1 < 1.1.1
			return true;

		if (lesserList[i].toInt() != greaterList[i].toInt())
			return lesserList[i].toInt() < greaterList[i].toInt(); // 1.1 < 1.2
	}
	return false;
}

CModEntry::CModEntry(QJsonObject repository, QJsonObject localData, QJsonValue modSettings, QString modname):
    repository(repository),
    localData(localData),
    modSettings(modSettings),
    modname(modname)
{
}

bool CModEntry::isEnabled() const
{
	if (!isInstalled())
		return false;

	return modSettings.toBool(false);
}

bool CModEntry::isDisabled() const
{
	if (!isInstalled())
		return false;
	return !isEnabled();
}

bool CModEntry::isAvailable() const
{
	if (isInstalled())
		return false;
	return !repository.isEmpty();
}

bool CModEntry::isUpdateable() const
{
	if (!isInstalled())
		return false;

	QString installedVer = localData["installedVersion"].toString();
	QString availableVer = repository["latestVersion"].toString();

	if (compareVersions(installedVer, availableVer))
		return true;
	return false;
}

bool CModEntry::isInstalled() const
{
	return !localData.isEmpty();
}

int CModEntry::getModStatus() const
{
	return
	(isEnabled()   ? ModStatus::ENABLED    : 0) |
	(isInstalled() ? ModStatus::INSTALLED  : 0) |
	(isUpdateable()? ModStatus::UPDATEABLE : 0);
}

QString CModEntry::getName() const
{
	return modname;
}

QVariant CModEntry::getValue(QString value) const
{
	if (repository.contains(value))
		return repository[value].toVariant();

	if (localData.contains(value))
		return localData[value].toVariant();

	return QVariant();
}

QJsonObject CModList::copyField(QJsonObject data, QString from, QString to)
{
	QJsonObject renamed;

	for (auto it = data.begin(); it != data.end(); it++)
	{
		QJsonObject object = it.value().toObject();

		object.insert(to, object.value(from));
		renamed.insert(it.key(), QJsonValue(object));
	}
	return renamed;
}

void CModList::addRepository(QJsonObject data)
{
	repositores.push_back(copyField(data, "version", "latestVersion"));
}

void CModList::setLocalModList(QJsonObject data)
{
	localModList = copyField(data, "version", "installedVersion");
}

void CModList::setModSettings(QJsonObject data)
{
	modSettings = data;
}

CModEntry CModList::getMod(QString modname) const
{
	assert(hasMod(modname));

	QJsonObject repo;
	QJsonObject local = localModList[modname].toObject();
	QJsonValue settings = modSettings[modname];

	for (auto entry : repositores)
	{
		if (entry.contains(modname))
		{
			if (repo.empty())
				repo = entry[modname].toObject();
			else
			{
				if (CModEntry::compareVersions(repo["version"].toString(),
				                               entry[modname].toObject()["version"].toString()))
					repo = entry[modname].toObject();
			}
		}
	}

	return CModEntry(repo, local, settings, modname);
}

bool CModList::hasMod(QString modname) const
{
	if (localModList.contains(modname))
		return true;

	for (auto entry : repositores)
		if (entry.contains(modname))
			return true;

	return false;
}

QStringList CModList::getRequirements(QString modname)
{
	QStringList ret;

	if (hasMod(modname))
	{
		auto mod = getMod(modname);

		for (auto entry : mod.getValue("depends").toStringList())
			ret += getRequirements(entry);
	}
	ret += modname;

	return ret;
}

QVector<QString> CModList::getModList() const
{
	QSet<QString> knownMods;
	QVector<QString> modList;
	for (auto repo : repositores)
	{
		for (auto it = repo.begin(); it != repo.end(); it++)
		{
			knownMods.insert(it.key());
		}
	}
	for (auto it = localModList.begin(); it != localModList.end(); it++)
	{
		knownMods.insert(it.key());
	}

	for (auto entry : knownMods)
	{
		modList.push_back(entry);
	}
	return modList;
}
