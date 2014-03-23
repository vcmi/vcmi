#include "StdInc.h"
#include "cmodlist.h"

#include "../../lib/JsonNode.h"
#include "../../lib/filesystem/CFileInputStream.h"

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

QString CModEntry::sizeToString(double size)
{
	static const QString sizes[] =
	{
		/*"%1 B", */"%1 KiB", "%1 MiB", "%1 GiB", "%1 TiB"
	};
	size_t index = 0;
	while (size > 1024 && index < 4)
	{
		size /= 1024;
		index++;
	}
	return sizes[index].arg(QString::number(size, 'f', 1));
}

CModEntry::CModEntry(QVariantMap repository, QVariantMap localData, QVariantMap modSettings, QString modname):
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

	return modSettings["active"].toBool();
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
	if (repository.contains(value) && localData.contains(value))
	{
		// value is present in both repo and locally installed. Select one from latest version
		QString installedVer = localData["installedVersion"].toString();
		QString availableVer = repository["latestVersion"].toString();

		if (compareVersions(installedVer, availableVer))
			return repository[value];
		else
			return localData[value];
	}

	if (repository.contains(value))
		return repository[value];

	if (localData.contains(value))
		return localData[value];

	return QVariant();
}

QVariantMap CModList::copyField(QVariantMap data, QString from, QString to)
{
	QVariantMap renamed;

	for (auto it = data.begin(); it != data.end(); it++)
	{
		QVariantMap modConf = it.value().toMap();

		modConf.insert(to, modConf.value(from));
		renamed.insert(it.key(), modConf);
	}
	return renamed;
}

void CModList::resetRepositories()
{
	repositories.clear();
}

void CModList::addRepository(QVariantMap data)
{
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

static QVariant getValue(QVariantMap input, QString path)
{
	if (path.size() > 1)
	{
		QString entryName = path.section('/', 0, 1);
		QString remainder = "/" + path.section('/', 2, -1);

		entryName.remove(0, 1);
		return getValue(input.value(entryName).toMap(), remainder);
	}
	else
	{
		return input;
	}
}

CModEntry CModList::getMod(QString modname) const
{
	QVariantMap repo;
	QVariantMap local = localModList[modname].toMap();
	QVariantMap settings;

	QString path = modname;
	path = "/" + path.replace(".", "/mods/");
	QVariant conf = getValue(modSettings, path);

	if (conf.isNull())
	{
		settings["active"] = true; // default
	}
	else
	{
		if (conf.canConvert<QVariantMap>())
			settings = conf.toMap();
		else
			settings.insert("active", conf);
	}

	for (auto entry : repositories)
	{
		QVariant repoVal = getValue(entry, path);
		if (repoVal.isValid())
		{
			if (repo.empty())
				repo = repoVal.toMap();
			else
			{
				if (CModEntry::compareVersions(repo["version"].toString(), repoVal.toMap()["version"].toString()))
					repo = repoVal.toMap();
			}
		}
	}

	return CModEntry(repo, local, settings, modname);
}

bool CModList::hasMod(QString modname) const
{
	if (localModList.contains(modname))
		return true;

	for (auto entry : repositories)
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
	for (auto repo : repositories)
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

QVector<QString> CModList::getChildren(QString parent) const
{
	QVector<QString> children;

	int depth = parent.count('.') + 1;
	for (const QString & mod : getModList())
	{
		if (mod.count('.') == depth && mod.startsWith(parent))
			children.push_back(mod);
	}
	return children;
}
