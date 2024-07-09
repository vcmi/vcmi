/*
 * cmodlist.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QVariantMap>
#include <QVariant>
#include <QVector>

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

	QVariant getValueImpl(QString value, bool localized) const;
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
	// vcmi essential files
	bool isEssential() const;
	// checks if version is compatible with vcmi
	bool isCompatible() const;
	// returns true if mod should be visible in Launcher
	bool isVisible() const;
	// returns true if mod type is Translation
	bool isTranslation() const;
	// returns true if mod type is Compatibility
	bool isCompatibilityPatch() const;
	// returns true if this is a submod
	bool isSubmod() const;

	// see ModStatus enum
	int getModStatus() const;

	QString getName() const;

	// get value of some field in mod structure. Returns empty optional if value is not present
	QVariant getValue(QString value) const;
	QVariant getBaseValue(QString value) const;

	QStringList getDependencies() const;
	QStringList getConflicts() const;

	static QString sizeToString(double size);
};

class CModList
{
	QVector<QVariantMap> repositories;
	QVariantMap localModList;
	QVariantMap modSettings;

	mutable QMap<QString, CModEntry> cachedMods;

	QVariantMap copyField(QVariantMap data, QString from, QString to) const;

	CModEntry getModUncached(QString modname) const;
public:
	virtual void resetRepositories();
	virtual void reloadRepositories();
	virtual void addRepository(QVariantMap data);
	virtual void setLocalModList(QVariantMap data);
	virtual void setModSettings(QVariant data);
	virtual void modChanged(QString modID);

	// returns mod by name. Note: mod MUST exist
	const CModEntry & getMod(QString modname) const;

	// returns list of all mods necessary to run selected one, including mod itself
	// order is: first mods in list don't have any dependencies, last mod is modname
	// note: may include mods not present in list
	QStringList getRequirements(QString modname);

	bool hasMod(QString modname) const;

	// returns list of all available mods
	QVector<QString> getModList() const;

	QVector<QString> getChildren(QString parent) const;
};
