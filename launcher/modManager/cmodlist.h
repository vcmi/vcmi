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

#include "StdInc.h"

#include <QVariantMap>
#include <QVariant>
#include <QVector>

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

VCMI_LIB_NAMESPACE_END

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
	// checks if verison is compatible with vcmi
	bool isCompatible() const;
	// returns if has any data
	bool isValid() const;
	// installed and enabled
	bool isTranslation() const;

	// see ModStatus enum
	int getModStatus() const;

	QString getName() const;

	// get value of some field in mod structure. Returns empty optional if value is not present
	QVariant getValue(QString value) const;
	QVariant getBaseValue(QString value) const;

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
	virtual void reloadRepositories();
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
