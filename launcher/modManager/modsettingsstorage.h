/*
 * modsettignsstorage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QVariantMap>

class ModSettingsStorage
{
	QVariantMap config;

	QString settingsPath() const;

	void setRootModActive(const QString & modName, bool on);
	void setModSettingActive(const QString & modName, bool on);

	QVariantMap getActivePresetData() const;
	QStringList getActiveMods() const;
	QVariantMap getModSettings(const QString & modName) const;

public:
	ModSettingsStorage();

	void setModActive(const QString & modName, bool on);
	void setActivePreset(const QString & presetName);

	QString getActivePreset() const;
	bool isModActive(const QString & modName) const;
	//QStringList getPresetsList() const;
};

