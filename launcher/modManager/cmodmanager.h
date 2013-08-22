#pragma once

#include "cmodlist.h"

class CModManager
{
	CModList * modList;

	QString settingsPath();

	// check-free version of public method
	bool doEnableMod(QString mod, bool on);
	bool doInstallMod(QString mod, QString archivePath);
	bool doUninstallMod(QString mod);

	QJsonObject modSettings;
	QJsonObject localMods;

public:
	CModManager(CModList * modList);

	void loadRepository(QString filename);
	void loadModSettings();
	void loadMods();

	/// mod management functions. Return true if operation was successful

	/// installs mod from zip archive located at archivePath
	bool installMod(QString mod, QString archivePath);
	bool uninstallMod(QString mod);
	bool enableMod(QString mod);
	bool disableMod(QString mod);

	bool canInstallMod(QString mod);
	bool canUninstallMod(QString mod);
	bool canEnableMod(QString mod);
	bool canDisableMod(QString mod);
};
