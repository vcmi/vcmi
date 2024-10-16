/*
 * modsettignsstorage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "modsettingsstorage.h"

#include "../../lib/VCMIDirs.h"
#include "../vcmiqt/jsonutils.h"

static QVariant writeValue(QString path, QVariantMap input, QVariant value)
{
	if(path.size() > 1)
	{
		QString entryName = path.section('/', 0, 1);
		QString remainder = "/" + path.section('/', 2, -1);
		entryName.remove(0, 1);
		input.insert(entryName, writeValue(remainder, input.value(entryName).toMap(), value));
		return input;
	}
	else
	{
		return value;
	}
}

void ModSettingsStorage::createInitialPreset()
{
	// TODO: scan mods directory for all its content?

	QStringList modList;
	QVariantMap preset;
	QVariantMap presetList;

	modList.push_back("vcmi");
	preset.insert("mods", modList);
	presetList.insert("default", preset);
	config.insert("presets", presetList);
}

void ModSettingsStorage::importInitialPreset()
{
	QStringList modList;
	QMap<QString, QVariantMap> modSettings;

	QVariantMap activeMods = config["activeMods"].toMap();
	for (QVariantMap::const_iterator modEntry = activeMods.begin(); modEntry != activeMods.end(); ++modEntry)
	{
		if (modEntry.value().toMap()["active"].toBool())
			modList.push_back(modEntry.key());

		QVariantMap submods = modEntry.value().toMap()["mods"].toMap();
		for (QVariantMap::const_iterator submodEntry = submods.begin(); submodEntry != submods.end(); ++submodEntry)
			modSettings[modEntry.key()].insert(submodEntry.key(), submodEntry.value().toMap()["active"]);
	}

	QVariantMap importedPreset;
	QVariantMap modSettingsVariant;
	QVariantMap presetList;

	for (QMap<QString, QVariantMap>::const_iterator modEntry = modSettings.begin(); modEntry != modSettings.end(); ++modEntry)
		modSettingsVariant.insert(modEntry.key(), modEntry.value());

	importedPreset.insert("mods", modList);
	importedPreset.insert("settings", modSettingsVariant);
	presetList.insert("default", importedPreset);
	config.insert("presets", presetList);
}

ModSettingsStorage::ModSettingsStorage()
{
	config = JsonUtils::JsonFromFile(settingsPath()).toMap();

	if (!config.contains("presets"))
	{
		config.insert("activePreset", QVariant("default"));
		if (config.contains("activeMods"))
			importInitialPreset(); // 1.5 format import
		else
			createInitialPreset(); // new install

		JsonUtils::JsonToFile(settingsPath(), config);
	}
}

QString ModSettingsStorage::settingsPath() const
{
	return pathToQString(VCMIDirs::get().userConfigPath() / "modSettings.json");
}

void ModSettingsStorage::setRootModActive(const QString & modName, bool on)
{
	QString presetName = getActivePreset();
	QStringList activeMods = getActiveMods();

	assert(modName.count('.') == 0); // this method should never be used for submods

	if (on)
		activeMods.push_back(modName);
	else
		activeMods.removeAll(modName);

	config = writeValue("/presets/" + presetName + "/mods", config, activeMods).toMap();

	JsonUtils::JsonToFile(settingsPath(), config);
}

void ModSettingsStorage::setModSettingActive(const QString & modName, bool on)
{
	QString presetName = getActivePreset();
	QString rootModName = modName.section('.', 0, 0);
	QString settingName = modName.section('.', 1);
	QVariantMap modSettings = getModSettings(rootModName);

	assert(modName.count('.') != 0); // this method should only be used for submods

	modSettings.insert(settingName, QVariant(on));

	config = writeValue("/presets/" + presetName + "/settings/" + rootModName, config, modSettings).toMap();

	JsonUtils::JsonToFile(settingsPath(), config);
}

void ModSettingsStorage::setModActive(const QString & modName, bool on)
{
	if (modName.contains('.'))
		setModSettingActive(modName, on);
	else
		setRootModActive(modName, on);
}

void ModSettingsStorage::setActivePreset(const QString & presetName)
{
	config.insert("activePreset", QVariant(presetName));
}

QString ModSettingsStorage::getActivePreset() const
{
	return config["activePreset"].toString();
}

bool ModSettingsStorage::isModActive(const QString & modName) const
{
	if (modName.contains('.'))
	{
		QString rootModName = modName.section('.', 0, 0);
		QString settingName = modName.section('.', 1);

		return getModSettings(rootModName)[settingName].toBool();
	}
	else
		return getActiveMods().contains(modName);
}

QStringList ModSettingsStorage::getActiveMods() const
{
	return getActivePresetData()["mods"].toStringList();
}

QVariantMap ModSettingsStorage::getActivePresetData() const
{
	QString presetName = getActivePreset();
	return config["presets"].toMap()[presetName].toMap();
}

QVariantMap ModSettingsStorage::getModSettings(const QString & modName) const
{
	QString presetName = getActivePreset();
	return getActivePresetData()["settings"].toMap()[modName].toMap();
}
