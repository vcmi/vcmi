#pragma once

#include <QVariantMap>
#include <QVariant>
#include <QVector>

class JsonNode;

namespace ModStatus
{
	enum EModStatus
	{
		MASK_NONE = 0,
		ENABLED = 1,
		INSTALLED = 2,
		UPDATEABLE = 4,
		MASK_ALL = 255
	};
}

class CModEntry
{
	// repository contains newest version only (if multiple are available)
	QVariantMap repository;
	QVariantMap localData;
	QVariantMap modSettings;

	QString modname;
public:
	CModEntry(QVariantMap repository, QVariantMap localData, QVariantMap modSettings, QString modname);

	// installed and enabled
	bool isEnabled() const;
	// installed but disabled
	bool isDisabled() const;
	// available in any of repositories but not installed
	bool isAvailable() const;
	// installed and greater version exists in repository
	bool isUpdateable() const;
	// installed
	bool isInstalled() const;

	// see ModStatus enum
	int getModStatus() const;

	QString getName() const;

	// get value of some field in mod structure. Returns empty optional if value is not present
	QVariant getValue(QString value) const;

	// returns true if less < greater comparing versions section by section
	static bool compareVersions(QString lesser, QString greater);

	static QString sizeToString(double size);
};

class CModList
{
	QVector<QVariantMap> repositories;
	QVariantMap localModList;
	QVariantMap modSettings;

	QVariantMap copyField(QVariantMap data, QString from, QString to);
public:
	virtual void resetRepositories();
	virtual void addRepository(QVariantMap data);
	virtual void setLocalModList(QVariantMap data);
	virtual void setModSettings(QVariant data);
	virtual void modChanged(QString modID);

	// returns mod by name. Note: mod MUST exist
	CModEntry getMod(QString modname) const;

	// returns list of all mods necessary to run selected one, including mod itself
	// order is: first mods in list don't have any dependencies, last mod is modname
	// note: may include mods not present in list
	QStringList getRequirements(QString modname);

	bool hasMod(QString modname) const;

	// returns list of all available mods
	QVector<QString> getModList() const;

	QVector<QString> getChildren(QString parent) const;
};
